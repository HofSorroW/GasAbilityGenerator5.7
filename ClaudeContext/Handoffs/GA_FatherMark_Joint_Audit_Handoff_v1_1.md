# GA_FatherMark Joint Audit Handoff v1.1
## Claude + GPT Cross-Examination Results
## Date: January 2026
## Status: RESEARCH PHASE - Contract Proposals Under Investigation

---

## Executive Summary

This document records the joint audit session between Claude (Opus 4.5) and GPT auditing GA_FatherMark.

**v1.1 Update:** Critical finding - manifest GA_FatherMark event graph is SIMPLIFIED compared to guide. Variables exist (MarkedEnemies, MaxMarks, MarkWidgetMap) but are NOT USED in the ability's event graph. This raises a fundamental question: is manifest incomplete or guide over-specified?

**Current Status:** Research phase on proposed contracts before any locking decisions.

---

## Audit Session Framework

### Rules Enforced
- Both sides act as auditors
- Challenge everything, accept nothing by default
- Validate against UE5.7, Narrative Pro v2.2, GAS, plugin code
- Research and web search when needed
- **NO CODING** until Erdem approves
- Check answers before asking questions
- All systems must align with Narrative Pro conventions

### Participants
| Auditor | Model | Role |
|---------|-------|------|
| Claude | Opus 4.5 | Primary auditor, source code access |
| GPT | GPT-4 | Cross-examiner, contract analysis |

---

## CRITICAL FINDING: Manifest vs Guide Implementation Gap

### What Manifest GA_FatherMark DOES Implement (lines 7549-7604)
1. ✅ Get PendingMarkTarget from BP_FatherCompanion
2. ✅ Copy to local TargetEnemy variable
3. ✅ Validate target (IsValid check)
4. ✅ MakeOutgoingGameplayEffectSpec for GE_MarkEffect
5. ✅ SetByCaller for Data.Mark.Duration
6. ✅ SetByCaller for Data.Mark.ArmorReduction
7. ✅ Apply GE to enemy ASC
8. ✅ EndAbility

### What Manifest GA_FatherMark does NOT Implement
| Feature | Guide v1.9 | Manifest | Status |
|---------|------------|----------|--------|
| Mark count check (MaxMarks) | ✅ Detailed | ❌ Missing | **GAP** |
| Oldest mark rotation | ✅ Detailed | ❌ Missing | **GAP** |
| Add to MarkedEnemies array | ✅ Detailed | ❌ Missing | **GAP** |
| Widget spawning (WBP_MarkIndicator) | ✅ Detailed | ❌ Missing | **GAP** |
| OnDied delegate binding | ✅ Detailed | ❌ Missing | **GAP** |
| Widget refresh on re-mark | ✅ Detailed | ❌ Missing | **GAP** |

### Variables Exist But Unused
BP_FatherCompanion has these variables defined (manifest lines 9571-9591):
```yaml
- name: PendingMarkTarget    # Used ✅
- name: MarkedEnemies        # Declared but NOT used by GA
- name: MaxMarks             # Declared but NOT used by GA
- name: MarkWidgetMap        # Declared but NOT used by GA
```

### Open Question for Erdem
**Which is authoritative for MISSING features?**
- A) Manifest is complete as-is → Guide over-specifies, simplify guide
- B) Guide is correct intent → Manifest needs enhancement
- C) Both need alignment to Design Doc

---

## Contract Proposals: Research Required

GPT proposed 4 contracts. Before locking, both auditors must research and agree on implementation patterns.

### Proposal 1: C_MARK_MAILBOX_SAFETY

**GPT's Proposal:**
> PASS if GA_FatherMark copies PendingMarkTarget into local TargetEnemy immediately AND clears/consumes the mailbox

**Claude's Challenge:**
- Manifest copies immediately but does NOT clear
- Is clearing necessary with ServerOnly + sequential tick?

**Research Required:**
- [ ] UE/GAS best practices for "mailbox" activation patterns
- [ ] Narrative Pro examples of similar patterns
- [ ] Risk analysis: when can overwrite actually occur?

---

### Proposal 2: C_MARK_MAX_CONCURRENCY_3

**GPT's Proposal:**
> PASS if NumValidMarkedEnemies <= 3 and removes oldest valid mark when exceeding

**Claude's Challenge:**
- Logic does NOT exist in manifest currently
- Would immediately FAIL current implementation

**Research Required:**
- [ ] UE/GAS patterns for limited concurrent effects
- [ ] Narrative Pro buff/debuff stack limits
- [ ] Standard rotation algorithms (FIFO, LRU, etc.)

---

### Proposal 3: C_MARK_REFRESH_SEMANTICS

**GPT's Proposal:**
> Re-application must refresh duration, refresh UI, not create duplicate widgets

**Claude's Challenge:**
- No widget logic in manifest GA_FatherMark
- GE stacking is set to "Refresh on Successful Application" in GE_MarkEffect

**Research Required:**
- [ ] UE GE stacking policies (Aggregate, Override, Refresh)
- [ ] Narrative Pro effect refresh patterns
- [ ] Widget component duplicate prevention patterns

---

### Proposal 4: C_MARK_UI_WALL_VISIBLE

**GPT's Proposal:**
> Widget must be configured for wall visibility (screen space or custom depth)

**Claude's Challenge:**
- Widget spawning not implemented in manifest
- Need to define what "wall visible" means technically

**Research Required:**
- [ ] UE Widget Component Space modes (World vs Screen)
- [ ] Custom Depth / Stencil approaches in UE5.7
- [ ] Narrative Pro marker/indicator patterns

---

## Confirmed Discrepancies (Both Auditors Agree)

### Finding 1: GE Structure Mismatch (HIGH)
| Guide v1.9 | Manifest (Authority) |
|------------|----------------------|
| 2 GEs: GE_FatherMark + GE_MarkDamageBonus | 1 GE: GE_MarkEffect |

**Status:** CONFIRMED ✅ - Guide must use single GE_MarkEffect

### Finding 2: Tag Namespace Mismatch (HIGH)
| Guide v1.9 | Manifest (Authority) |
|------------|----------------------|
| `Enemy.State.Marked` | `Character.Marked` |

**Status:** CONFIRMED ✅ - Guide must use Character.Marked

### Finding 3: Missing Blocked Tag (MEDIUM)
| Guide v1.9 | Manifest (Authority) |
|------------|----------------------|
| Missing | `Father.State.Transitioning` |

**Status:** CONFIRMED ✅ - Guide must add this tag

### Finding 4: Variable Structure (MEDIUM)
| Guide v1.9 | Manifest (Authority) |
|------------|----------------------|
| FatherRef, TargetEnemy, MarkWidgetRef | TargetEnemy, MarkDuration, MarkArmorReduction |

**Status:** CONFIRMED ✅ - Guide must use SetByCaller variables

### Finding 5: Armor Math Heuristic (LOW)
| Claim | Reality |
|-------|---------|
| "-10 Armor = ~10% more damage" | Conditional: ~10% only for 10-armor targets |

**Status:** CONFIRMED ✅ - Documentation should clarify conditional nature

---

## Contract Compliance Summary

| Contract | Status | Notes |
|----------|--------|-------|
| Contract 24 | ✅ N/A | GE_MarkEffect is debuff, not damage |
| Contract 24A | ✅ COMPLIANT | SetByCaller for duration/armor (allowed) |
| Contract 27 | ✅ N/A | No cooldown (passive ability) |
| R-DELEGATE-1 | ⚠️ UNTESTED | OnDied binding not in manifest |

---

## Research Agenda

### Phase 1: UE/Epic Standards Research
1. GAS effect stacking and refresh policies
2. Widget Component visibility modes
3. Array-based tracking patterns in GAS

### Phase 2: Narrative Pro Code Analysis
1. Search for similar marking/tracking systems
2. Examine buff/debuff stack limit implementations
3. Review UI indicator patterns

### Phase 3: Cross-Auditor Agreement
1. Present findings to GPT
2. Agree on implementation approach
3. Then discuss locking

---

## Session Commits

| Commit | Hash | Description |
|--------|------|-------------|
| GA_DomeBurst fixes | `541a5f9` | Contract 24/24A + Contract 27 compliance |
| GA_FatherLaserShot fixes | `4d8c444` | Tag hierarchy, cooldown, version |
| Initial audit handoff | `61ffa11` | First audit findings |
| Joint audit v1.0 | `9d39ce5` | Claude + GPT consensus |
| Joint audit v1.1 | (pending) | Manifest gap findings + research agenda |

---

**END OF JOINT AUDIT HANDOFF v1.1**

**Status:** Research Phase - Investigating contract proposals
**Next Action:** Research UE/Narrative Pro patterns, then cross-examine with GPT
