#!/usr/bin/env python3
"""
Form State Architecture Lint Rules (v4.13.4)

Validates manifest.yaml for proper Option B form state implementation.
Run after manifest changes to prevent regressions.

Usage:
    python validate_form_state_architecture.py [--manifest PATH]

Rules:
    L1: Every form ability must apply its FormState GE
    L2: Every form must remove prior FormState on switch path
    L3: Ban removal by Narrative.State.Invulnerable tag
    L4: Required-tags closure (blast radius prevention)
"""

try:
    import yaml
except ImportError:
    print("ERROR: PyYAML not installed. Install with: pip install pyyaml")
    print("Or run: python -m pip install pyyaml")
    import sys
    sys.exit(1)

import sys
import os
import re
from pathlib import Path
from typing import Dict, List, Set, Tuple, Optional
from dataclasses import dataclass

# Form ability to GE mapping (L1)
FORM_GE_MAPPING = {
    "GA_FatherCrawler": "GE_CrawlerState",
    "GA_FatherArmor": "GE_ArmorState",
    "GA_FatherExoskeleton": "GE_ExoskeletonState",
    "GA_FatherSymbiote": "GE_SymbioteState",
    "GA_FatherEngineer": "GE_EngineerState",
}

# GE to granted tag mapping (for L4 closure check)
GE_TAG_MAPPING = {
    "GE_CrawlerState": "Effect.Father.FormState.Crawler",
    "GE_ArmorState": "Effect.Father.FormState.Armor",
    "GE_ExoskeletonState": "Effect.Father.FormState.Exoskeleton",
    "GE_SymbioteState": "Effect.Father.FormState.Symbiote",
    "GE_EngineerState": "Effect.Father.FormState.Engineer",
}

# Banned tags for removal (L3)
BANNED_REMOVAL_TAGS = [
    "Narrative.State.Invulnerable",
]

@dataclass
class LintError:
    rule: str
    ability: str
    message: str
    severity: str = "ERROR"

def load_manifest(path: str) -> dict:
    """Load and parse manifest.yaml, merging duplicate keys"""
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Handle duplicate keys by merging arrays
    # Split into documents and merge
    result = {}

    # Use yaml.load_all for potential multi-document YAML
    docs = list(yaml.safe_load_all(content))

    # If single document, just return it
    if len(docs) == 1 and docs[0]:
        return docs[0]

    # Multiple documents - merge them
    for doc in docs:
        if doc:
            for key, value in doc.items():
                if key in result and isinstance(value, list) and isinstance(result[key], list):
                    result[key].extend(value)
                else:
                    result[key] = value

    return result if result else docs[0] if docs else {}

def load_manifest_raw(path: str) -> str:
    """Load raw manifest content for text-based searching"""
    with open(path, 'r', encoding='utf-8') as f:
        return f.read()

def find_ability_in_raw(content: str, ability_name: str) -> Optional[str]:
    """Find ability block in raw manifest content"""
    # Look for the ability definition
    pattern = rf'- name: {ability_name}\s'
    match = re.search(pattern, content)
    if not match:
        return None

    # Extract the ability block (until next "- name:" or end of section)
    start = match.start()
    # Find the end - next "- name:" at same indent level or section header
    rest = content[start:]

    # Find next ability or section
    next_ability = re.search(r'\n  - name: ', rest[10:])  # Skip current "- name:"
    if next_ability:
        end = start + 10 + next_ability.start()
    else:
        end = len(content)

    return content[start:end]

def find_node_by_id(nodes: List[dict], node_id: str) -> Optional[dict]:
    """Find a node by its ID"""
    for node in nodes:
        if node.get('id') == node_id:
            return node
    return None

def check_applies_ge(ability: dict, expected_ge: str) -> Tuple[bool, Optional[str]]:
    """
    L1: Check if ability applies the expected GE_*State.
    Returns (success, error_message)
    """
    event_graph = ability.get('event_graph', {})
    nodes = event_graph.get('nodes', [])

    for node in nodes:
        props = node.get('properties', {})
        if (props.get('function') == 'ApplyGameplayEffectToSelf' and
            props.get('gameplay_effect_class') == expected_ge):
            return True, None

    return False, f"Missing ApplyGameplayEffectToSelf node with {expected_ge}"

def check_removes_prior_form_state(ability: dict) -> Tuple[bool, Optional[str]]:
    """
    L2: Check if ability removes prior form state using Effect.Father.FormState tag.
    Returns (success, error_message)
    """
    event_graph = ability.get('event_graph', {})
    nodes = event_graph.get('nodes', [])
    connections = event_graph.get('connections', [])

    # Look for RemoveGameplayEffectFromOwnerWithGrantedTags node
    has_remove_node = False
    has_tag_container = False
    has_literal_tag = False

    for node in nodes:
        props = node.get('properties', {})

        if props.get('function') == 'RemoveGameplayEffectFromOwnerWithGrantedTags':
            has_remove_node = True

        if (props.get('function') == 'MakeLiteralGameplayTag' and
            props.get('literal_value') == 'Effect.Father.FormState'):
            has_literal_tag = True

        if props.get('function') == 'MakeGameplayTagContainerFromTag':
            has_tag_container = True

    if not has_remove_node:
        return False, "Missing RemoveGameplayEffectFromOwnerWithGrantedTags node"

    if not has_literal_tag:
        return False, "Missing MakeLiteralGameplayTag with Effect.Father.FormState"

    if not has_tag_container:
        return False, "Missing MakeGameplayTagContainerFromTag node"

    return True, None

def check_banned_removal_tags(ability: dict) -> List[str]:
    """
    L3: Check for banned removal tag patterns.
    Returns list of violations.
    """
    violations = []
    event_graph = ability.get('event_graph', {})
    nodes = event_graph.get('nodes', [])
    connections = event_graph.get('connections', [])

    for node in nodes:
        props = node.get('properties', {})

        # Check MakeLiteralGameplayTag for banned tags
        if props.get('function') == 'MakeLiteralGameplayTag':
            literal_value = props.get('literal_value', '')
            for banned in BANNED_REMOVAL_TAGS:
                if literal_value == banned:
                    # Check if this feeds into a removal function
                    node_id = node.get('id')
                    for conn in connections:
                        if conn.get('from', [None])[0] == node_id:
                            # This tag is being used somewhere
                            violations.append(
                                f"Uses banned tag '{banned}' in {node_id}"
                            )

    return violations

def check_required_tags_closure(manifest: dict) -> List[LintError]:
    """
    L4: For abilities with activation_required_tags containing Effect.Father.FormState.*,
    verify the corresponding GE exists and some form ability applies it.
    """
    errors = []
    abilities = manifest.get('gameplay_abilities', [])
    effects = manifest.get('gameplay_effects', [])

    # Build set of available GEs
    available_ges = {ge.get('name') for ge in effects}

    # Build set of form-state tags that are applied by form abilities
    applied_tags = set()
    for ability in abilities:
        name = ability.get('name', '')
        if name in FORM_GE_MAPPING:
            ge_name = FORM_GE_MAPPING[name]
            if ge_name in GE_TAG_MAPPING:
                applied_tags.add(GE_TAG_MAPPING[ge_name])

    # Check each ability for required form state tags
    for ability in abilities:
        name = ability.get('name', '')
        tags = ability.get('tags', {})
        required_tags = tags.get('activation_required_tags', [])

        for tag in required_tags:
            if tag.startswith('Effect.Father.FormState.'):
                # Find which GE grants this tag
                granting_ge = None
                for ge_name, granted_tag in GE_TAG_MAPPING.items():
                    if granted_tag == tag:
                        granting_ge = ge_name
                        break

                if granting_ge is None:
                    errors.append(LintError(
                        rule="L4",
                        ability=name,
                        message=f"Required tag '{tag}' has no known granting GE",
                        severity="ERROR"
                    ))
                elif granting_ge not in available_ges:
                    errors.append(LintError(
                        rule="L4",
                        ability=name,
                        message=f"Required tag '{tag}' granted by {granting_ge} which doesn't exist",
                        severity="ERROR"
                    ))
                elif tag not in applied_tags:
                    errors.append(LintError(
                        rule="L4",
                        ability=name,
                        message=f"Required tag '{tag}' is never applied (no form ability applies {granting_ge})",
                        severity="ERROR"
                    ))

    return errors

def validate_manifest(manifest_path: str) -> List[LintError]:
    """Run all lint rules and return errors using text-based search"""
    errors = []

    try:
        raw_content = load_manifest_raw(manifest_path)
    except Exception as e:
        return [LintError("PARSE", "manifest.yaml", f"Failed to read: {e}")]

    # L1 & L2 & L3: Check each form ability using text search
    for form_ability, expected_ge in FORM_GE_MAPPING.items():
        ability_block = find_ability_in_raw(raw_content, form_ability)

        if not ability_block:
            errors.append(LintError(
                rule="L1",
                ability=form_ability,
                message=f"Form ability not found in manifest"
            ))
            continue

        # L1: Check GE application - look for ApplyGameplayEffectToSelf with the expected GE
        l1_pattern = rf'function: ApplyGameplayEffectToSelf.*?gameplay_effect_class: {expected_ge}'
        if not re.search(l1_pattern, ability_block, re.DOTALL):
            errors.append(LintError(
                rule="L1",
                ability=form_ability,
                message=f"Missing ApplyGameplayEffectToSelf node with {expected_ge}"
            ))

        # L2: Check prior form state removal
        has_remove_func = 'function: RemoveGameplayEffectFromOwnerWithGrantedTags' in ability_block
        has_form_state_tag = 'literal_value: Effect.Father.FormState' in ability_block
        has_tag_container = 'function: MakeGameplayTagContainerFromTag' in ability_block

        if not has_remove_func:
            errors.append(LintError(
                rule="L2",
                ability=form_ability,
                message="Missing RemoveGameplayEffectFromOwnerWithGrantedTags node"
            ))
        elif not has_form_state_tag:
            errors.append(LintError(
                rule="L2",
                ability=form_ability,
                message="Missing MakeLiteralGameplayTag with Effect.Father.FormState"
            ))
        elif not has_tag_container:
            errors.append(LintError(
                rule="L2",
                ability=form_ability,
                message="Missing MakeGameplayTagContainerFromTag node"
            ))

        # L3: Check for banned removal tags
        for banned_tag in BANNED_REMOVAL_TAGS:
            if f'literal_value: {banned_tag}' in ability_block:
                errors.append(LintError(
                    rule="L3",
                    ability=form_ability,
                    message=f"Uses banned tag '{banned_tag}' for removal"
                ))

    # L4: Required tags closure - check all abilities with form state requirements
    l4_pattern = r'- name: (\S+).*?activation_required_tags:.*?(Effect\.Father\.FormState\.\w+)'
    for match in re.finditer(l4_pattern, raw_content, re.DOTALL):
        ability_name = match.group(1)
        required_tag = match.group(2)

        # Find which GE grants this tag
        granting_ge = None
        for ge_name, granted_tag in GE_TAG_MAPPING.items():
            if granted_tag == required_tag:
                granting_ge = ge_name
                break

        if granting_ge is None:
            errors.append(LintError(
                rule="L4",
                ability=ability_name,
                message=f"Required tag '{required_tag}' has no known granting GE"
            ))
        elif f'name: {granting_ge}' not in raw_content:
            errors.append(LintError(
                rule="L4",
                ability=ability_name,
                message=f"Required tag '{required_tag}' granted by {granting_ge} which doesn't exist"
            ))
        else:
            # Check if some form ability applies this GE
            ge_applied = False
            for form_name, ge_name in FORM_GE_MAPPING.items():
                if ge_name == granting_ge:
                    form_block = find_ability_in_raw(raw_content, form_name)
                    if form_block and f'gameplay_effect_class: {ge_name}' in form_block:
                        ge_applied = True
                        break
            if not ge_applied:
                errors.append(LintError(
                    rule="L4",
                    ability=ability_name,
                    message=f"Required tag '{required_tag}' is never applied (no form applies {granting_ge})"
                ))

    return errors

def main():
    # Default manifest path
    script_dir = Path(__file__).parent
    default_manifest = script_dir.parent / "ClaudeContext" / "manifest.yaml"

    # Check for command line override
    manifest_path = str(default_manifest)
    if len(sys.argv) > 1:
        if sys.argv[1] == '--manifest' and len(sys.argv) > 2:
            manifest_path = sys.argv[2]
        elif sys.argv[1] in ['-h', '--help']:
            print(__doc__)
            sys.exit(0)

    if not os.path.exists(manifest_path):
        print(f"ERROR: Manifest not found at {manifest_path}")
        sys.exit(1)

    print(f"=== Form State Architecture Lint (v4.13.4) ===")
    print(f"Manifest: {manifest_path}")
    print()

    errors = validate_manifest(manifest_path)

    if not errors:
        print("PASS: All lint rules passed")
        print()
        print("Checked:")
        print("  L1: Form abilities apply their GE_*State")
        print("  L2: Form abilities remove prior FormState")
        print("  L3: No banned Narrative.State.Invulnerable removal")
        print("  L4: Required tags have granting forms")
        sys.exit(0)
    else:
        print(f"FAIL: {len(errors)} error(s) found")
        print()
        for error in errors:
            print(f"  [{error.rule}] {error.ability}: {error.message}")
        print()
        print("See ClaudeContext/Handoffs/Form_State_Architecture_Fix_v4.13.2.md for details")
        sys.exit(1)

if __name__ == '__main__':
    main()
