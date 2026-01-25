# Stalker System Full Compliance Implementation Plan

## VERSION 1.0.1 - COMPREHENSIVE IMPLEMENTATION (AUDIT PATCHED)

## Audit Patches Applied (v1.0.1)
- **PATCH-B**: Fixed delegate signature to match Narrative Pro `FOnDamagedBy(ASC, Float, Spec)`
- **PATCH-C**: Added State Tag Contract section with apply/remove rules
- **PATCH-D**: Split cleanup into `StopFollowing()` and `BecomeAggressive()` for permanent aggro

## Decisions Applied
- Dialogue content: Placeholder lines
- Defend goal: Handle both OnGoalSucceeded AND OnGoalFailed
- Faction: New "Returned" faction with attitudes
- BP_ReturnedStalker: Full setup logic

---

## PART 1: TAGS (Already Present + New Faction)

### Existing Tags (Verified in manifest lines 10343-10365)
```yaml
tags:  # STALKER - EXISTING
  - tag: State.NPC.Returned.Following
    comment: Applied during follow state
  - tag: State.NPC.Returned.Bonded
    comment: Applied when TalkCount > 0
  - tag: State.NPC.Returned.Defending
    comment: Applied while defending player
  - tag: State.NPC.Returned.Aggressive
    comment: Applied after attacking player
  - tag: Narrative.TaggedDialogue.Returned.StartFollowing
    comment: When follow begins
  - tag: Narrative.TaggedDialogue.Returned.Following.First
    comment: First talk (TalkCount = 1)
  - tag: Narrative.TaggedDialogue.Returned.Following.Bonding
    comment: TalkCount 2-3
  - tag: Narrative.TaggedDialogue.Returned.Following.Deep
    comment: TalkCount 4+
  - tag: Narrative.TaggedDialogue.Returned.Defending
    comment: When defending player
  - tag: Narrative.TaggedDialogue.Returned.Attacking
    comment: When attacking player
  - tag: Narrative.TaggedDialogue.Returned.GivingUp
    comment: When timeout expires
```

### New Faction Tag (ADD)
```yaml
tags:  # STALKER - NEW FACTION
  - tag: Faction.NPC.Returned
    comment: Returned creatures - neutral until provoked or bonded
```

---

## PART 2: FACTION ATTITUDES (NEW)

Add to project settings or faction configuration:

| Faction A | Faction B | Attitude | Notes |
|-----------|-----------|----------|-------|
| Faction.NPC.Returned | Narrative.Factions.Heroes | Neutral | Default - can become friendly via bond or hostile via attack |
| Faction.NPC.Returned | Faction.NPC.Returned | Friendly | Don't attack each other |
| Faction.NPC.Returned | Narrative.Factions.Bandits | Hostile | Will defend player against bandits |
| Faction.NPC.Returned | Narrative.Factions.Monsters | Hostile | Will defend player against monsters |

---

## PART 2.5: STATE TAG CONTRACT (AUDIT PATCH-C)

State gameplay tags must be applied/removed at specific transition points per guide compliance.

### State Tag Transition Table

| Transition | Tag Action | Location |
|------------|------------|----------|
| **Follow Starts** | ADD `State.NPC.Returned.Following` | OnFollowCheckTimer (after Add Goal) |
| **First Talk** | ADD `State.NPC.Returned.Bonded` | OnTalkCheckTimer (when TalkCount becomes 1) |
| **Defend Starts** | ADD `State.NPC.Returned.Defending` | OnPlayerDamaged (after set bIsDefending) |
| **Defend Ends** | REMOVE `State.NPC.Returned.Defending` | OnDefendGoalCompleted |
| **Attack Player** | ADD `State.NPC.Returned.Aggressive` | OnAttackCheckTimer (PERMANENT - never removed) |
| **Timeout/GiveUp** | REMOVE `Following`, `Bonded`, `Defending` | StopFollowing() |
| **Become Aggressive** | REMOVE `Following`, `Bonded`; KEEP `Aggressive` | BecomeAggressive() |

### Tag Application Pattern (Blueprint nodes)

```yaml
# Pattern for adding state tag
- id: GetASC
  type: CallFunction
  properties:
    function: GetAbilitySystemComponent
- id: AddStateTag
  type: CallFunction
  properties:
    function: AddLooseGameplayTag
    parameters:
      tag: "State.NPC.Returned.Following"  # or Bonded/Defending/Aggressive

# Pattern for removing state tag
- id: RemoveStateTag
  type: CallFunction
  properties:
    function: RemoveLooseGameplayTag
    parameters:
      tag: "State.NPC.Returned.Following"
```

### Critical Rule: Aggressive is PERMANENT

Once `State.NPC.Returned.Aggressive` is applied (attack roll success), it is **NEVER removed**. The NPC remains hostile to the player permanently. This is why we need separate cleanup functions:
- `StopFollowing()` - for timeout, removes Following/Bonded/Defending, returns to patrol
- `BecomeAggressive()` - for attack success, removes Following/Bonded, adds Aggressive, stays hostile

---

## PART 3: GOALGENERATOR_RANDOMAGGRESSION (COMPLETE REWRITE)

### 3.1 Variables (35 Total)

```yaml
actor_blueprints:  # STALKER GOAL GENERATORS
  - name: GoalGenerator_RandomAggression
    folder: AI/GoalGenerators
    parent_class: NPCGoalGenerator
    variables:
      # ═══════════════════════════════════════════════════════════════
      # CONFIGURATION VARIABLES (17) - Instance Editable
      # ═══════════════════════════════════════════════════════════════

      # Follow Detection
      - name: FollowCheckInterval
        type: Float
        default_value: "8.0"
        instance_editable: true
        comment: Seconds between follow chance rolls
      - name: FollowChance
        type: Float
        default_value: "0.15"
        instance_editable: true
        comment: Probability to start following (15%)
      - name: DetectionRange
        type: Float
        default_value: "2000.0"
        instance_editable: true
        comment: Max distance to detect player
      - name: FollowDistance
        type: Float
        default_value: "400.0"
        instance_editable: true
        comment: Distance to maintain while following

      # Follow Duration
      - name: BaseFollowDuration
        type: Float
        default_value: "20.0"
        instance_editable: true
        comment: Base timeout before giving up (seconds)
      - name: FollowDurationPerStack
        type: Float
        default_value: "5.0"
        instance_editable: true
        comment: Extra duration per TalkCount

      # Talk/Bond System
      - name: TalkCheckInterval
        type: Float
        default_value: "8.0"
        instance_editable: true
        comment: Seconds between talk rolls
      - name: TalkChance
        type: Float
        default_value: "0.3"
        instance_editable: true
        comment: Probability of talk success (30%)
      - name: MaxTalkStacks
        type: Integer
        default_value: "5"
        instance_editable: true
        comment: Maximum bond level

      # Attack System
      - name: AttackCheckInterval
        type: Float
        default_value: "3.0"
        instance_editable: true
        comment: Seconds between attack rolls
      - name: BaseAttackChance
        type: Float
        default_value: "0.25"
        instance_editable: true
        comment: Base attack probability (25%)
      - name: AttackReductionPerStack
        type: Float
        default_value: "0.05"
        instance_editable: true
        comment: Attack chance reduction per TalkCount
      - name: MinAttackChance
        type: Float
        default_value: "0.025"
        instance_editable: true
        comment: Minimum attack chance (2.5%)

      # Defend System
      - name: BaseDefendChance
        type: Float
        default_value: "0.5"
        instance_editable: true
        comment: Base defend probability (50%)
      - name: DefendBonusPerStack
        type: Float
        default_value: "0.05"
        instance_editable: true
        comment: Defend chance bonus per TalkCount
      - name: MaxDefendChance
        type: Float
        default_value: "0.75"
        instance_editable: true
        comment: Maximum defend chance (75%)
      - name: DefendRange
        type: Float
        default_value: "1500.0"
        instance_editable: true
        comment: Max distance to attacker to trigger defend

      # ═══════════════════════════════════════════════════════════════
      # RUNTIME STATE VARIABLES (7) - Not Editable
      # ═══════════════════════════════════════════════════════════════
      - name: CurrentFollowGoal
        type: Object
        class: Goal_FollowCharacter
        comment: Active follow goal reference
      - name: CurrentDefendGoal
        type: Object
        class: Goal_Attack
        comment: Active defend goal reference
      - name: FollowStartTime
        type: Float
        default_value: "0.0"
        comment: Timestamp when following started
      - name: TalkCount
        type: Integer
        default_value: "0"
        comment: Current bond level (0-5)
      - name: bIsFollowing
        type: Boolean
        default_value: "false"
        comment: Currently following player
      - name: bIsDefending
        type: Boolean
        default_value: "false"
        comment: Currently defending player
      - name: CachedPlayerCharacter
        type: Object
        class: NarrativePlayerCharacter
        comment: Cached player reference for unbinding

      # ═══════════════════════════════════════════════════════════════
      # TIMER HANDLE VARIABLES (4)
      # ═══════════════════════════════════════════════════════════════
      - name: FollowCheckTimerHandle
        type: TimerHandle
        comment: Periodic follow chance check
      - name: TalkCheckTimerHandle
        type: TimerHandle
        comment: Periodic talk roll
      - name: AttackCheckTimerHandle
        type: TimerHandle
        comment: Periodic attack roll
      - name: FollowDurationTimerHandle
        type: TimerHandle
        comment: Timeout for giving up

      # ═══════════════════════════════════════════════════════════════
      # TAGGED DIALOGUE TAG VARIABLES (7) - Instance Editable
      # ═══════════════════════════════════════════════════════════════
      - name: Tag_StartFollowing
        type: GameplayTag
        default_value: "Narrative.TaggedDialogue.Returned.StartFollowing"
        instance_editable: true
      - name: Tag_FollowFirst
        type: GameplayTag
        default_value: "Narrative.TaggedDialogue.Returned.Following.First"
        instance_editable: true
      - name: Tag_FollowBonding
        type: GameplayTag
        default_value: "Narrative.TaggedDialogue.Returned.Following.Bonding"
        instance_editable: true
      - name: Tag_FollowDeep
        type: GameplayTag
        default_value: "Narrative.TaggedDialogue.Returned.Following.Deep"
        instance_editable: true
      - name: Tag_Defending
        type: GameplayTag
        default_value: "Narrative.TaggedDialogue.Returned.Defending"
        instance_editable: true
      - name: Tag_Attacking
        type: GameplayTag
        default_value: "Narrative.TaggedDialogue.Returned.Attacking"
        instance_editable: true
      - name: Tag_GivingUp
        type: GameplayTag
        default_value: "Narrative.TaggedDialogue.Returned.GivingUp"
        instance_editable: true
```

### 3.2 Function Overrides

```yaml
    # ═══════════════════════════════════════════════════════════════
    # FUNCTION OVERRIDES
    # ═══════════════════════════════════════════════════════════════
    function_overrides:
      - function: InitializeGoalGenerator
        nodes:
          # Call parent first
          - id: CallParent
            type: CallParent
            position: [200, 0]

          # Get interval for timer
          - id: GetFollowCheckInterval
            type: VariableGet
            properties:
              variable_name: FollowCheckInterval
            position: [400, 100]

          # Set up repeating timer for follow check
          - id: SetTimer_FollowCheck
            type: CallFunction
            properties:
              function: K2_SetTimer
              class: KismetSystemLibrary
              target_self: true
              parameters:
                FunctionName: "OnFollowCheckTimer"
                bLooping: true
            position: [600, 0]

          # Store timer handle
          - id: SetFollowCheckHandle
            type: VariableSet
            properties:
              variable_name: FollowCheckTimerHandle
            position: [800, 0]

          # Debug print
          - id: PrintInit
            type: PrintString
            properties:
              message: "GoalGenerator_RandomAggression: Initialized with bond system"
            position: [1000, 0]

        connections:
          - from: [FunctionEntry, Then]
            to: [CallParent, Exec]
          - from: [CallParent, Exec]
            to: [SetTimer_FollowCheck, Exec]
          - from: [GetFollowCheckInterval, FollowCheckInterval]
            to: [SetTimer_FollowCheck, Time]
          - from: [SetTimer_FollowCheck, ReturnValue]
            to: [SetFollowCheckHandle, FollowCheckTimerHandle]
          - from: [SetTimer_FollowCheck, Then]
            to: [SetFollowCheckHandle, Exec]
          - from: [SetFollowCheckHandle, Then]
            to: [PrintInit, Exec]
```

### 3.3 Custom Functions (5) - AUDIT PATCHED

```yaml
    # ═══════════════════════════════════════════════════════════════
    # CUSTOM FUNCTIONS
    # ═══════════════════════════════════════════════════════════════
    functions:
      # ─────────────────────────────────────────────────────────────
      # GetCurrentFollowDuration - Calculate duration based on TalkCount
      # Formula: BaseFollowDuration + (TalkCount * FollowDurationPerStack)
      # ─────────────────────────────────────────────────────────────
      - name: GetCurrentFollowDuration
        return_type: Float
        nodes:
          - id: GetTalkCount
            type: VariableGet
            properties:
              variable_name: TalkCount
            position: [200, 0]
          - id: ToFloat
            type: CallFunction
            properties:
              function: Conv_IntToFloat
              class: KismetMathLibrary
            position: [400, 0]
          - id: GetDurationPerStack
            type: VariableGet
            properties:
              variable_name: FollowDurationPerStack
            position: [400, 100]
          - id: Multiply
            type: CallFunction
            properties:
              function: Multiply_FloatFloat
              class: KismetMathLibrary
            position: [600, 0]
          - id: GetBaseDuration
            type: VariableGet
            properties:
              variable_name: BaseFollowDuration
            position: [600, 100]
          - id: Add
            type: CallFunction
            properties:
              function: Add_FloatFloat
              class: KismetMathLibrary
            position: [800, 0]
          - id: Return
            type: Return
            position: [1000, 0]
        connections:
          - from: [GetTalkCount, TalkCount]
            to: [ToFloat, InInt]
          - from: [ToFloat, ReturnValue]
            to: [Multiply, A]
          - from: [GetDurationPerStack, FollowDurationPerStack]
            to: [Multiply, B]
          - from: [GetBaseDuration, BaseFollowDuration]
            to: [Add, A]
          - from: [Multiply, ReturnValue]
            to: [Add, B]
          - from: [Add, ReturnValue]
            to: [Return, ReturnValue]

      # ─────────────────────────────────────────────────────────────
      # GetCurrentAttackChance - Calculate attack chance (decreases with bond)
      # Formula: Max(BaseAttackChance - (TalkCount * AttackReductionPerStack), MinAttackChance)
      # ─────────────────────────────────────────────────────────────
      - name: GetCurrentAttackChance
        return_type: Float
        nodes:
          - id: GetTalkCount
            type: VariableGet
            properties:
              variable_name: TalkCount
            position: [200, 0]
          - id: ToFloat
            type: CallFunction
            properties:
              function: Conv_IntToFloat
              class: KismetMathLibrary
            position: [400, 0]
          - id: GetReduction
            type: VariableGet
            properties:
              variable_name: AttackReductionPerStack
            position: [400, 100]
          - id: Multiply
            type: CallFunction
            properties:
              function: Multiply_FloatFloat
              class: KismetMathLibrary
            position: [600, 0]
          - id: GetBaseChance
            type: VariableGet
            properties:
              variable_name: BaseAttackChance
            position: [600, 100]
          - id: Subtract
            type: CallFunction
            properties:
              function: Subtract_FloatFloat
              class: KismetMathLibrary
            position: [800, 0]
          - id: GetMinChance
            type: VariableGet
            properties:
              variable_name: MinAttackChance
            position: [800, 100]
          - id: Max
            type: CallFunction
            properties:
              function: FMax
              class: KismetMathLibrary
            position: [1000, 0]
          - id: Return
            type: Return
            position: [1200, 0]
        connections:
          - from: [GetTalkCount, TalkCount]
            to: [ToFloat, InInt]
          - from: [ToFloat, ReturnValue]
            to: [Multiply, A]
          - from: [GetReduction, AttackReductionPerStack]
            to: [Multiply, B]
          - from: [GetBaseChance, BaseAttackChance]
            to: [Subtract, A]
          - from: [Multiply, ReturnValue]
            to: [Subtract, B]
          - from: [Subtract, ReturnValue]
            to: [Max, A]
          - from: [GetMinChance, MinAttackChance]
            to: [Max, B]
          - from: [Max, ReturnValue]
            to: [Return, ReturnValue]

      # ─────────────────────────────────────────────────────────────
      # GetCurrentDefendChance - Calculate defend chance (increases with bond)
      # Formula: Min(BaseDefendChance + (TalkCount * DefendBonusPerStack), MaxDefendChance)
      # ─────────────────────────────────────────────────────────────
      - name: GetCurrentDefendChance
        return_type: Float
        nodes:
          - id: GetTalkCount
            type: VariableGet
            properties:
              variable_name: TalkCount
            position: [200, 0]
          - id: ToFloat
            type: CallFunction
            properties:
              function: Conv_IntToFloat
              class: KismetMathLibrary
            position: [400, 0]
          - id: GetBonus
            type: VariableGet
            properties:
              variable_name: DefendBonusPerStack
            position: [400, 100]
          - id: Multiply
            type: CallFunction
            properties:
              function: Multiply_FloatFloat
              class: KismetMathLibrary
            position: [600, 0]
          - id: GetBaseChance
            type: VariableGet
            properties:
              variable_name: BaseDefendChance
            position: [600, 100]
          - id: Add
            type: CallFunction
            properties:
              function: Add_FloatFloat
              class: KismetMathLibrary
            position: [800, 0]
          - id: GetMaxChance
            type: VariableGet
            properties:
              variable_name: MaxDefendChance
            position: [800, 100]
          - id: Min
            type: CallFunction
            properties:
              function: FMin
              class: KismetMathLibrary
            position: [1000, 0]
          - id: Return
            type: Return
            position: [1200, 0]
        connections:
          - from: [GetTalkCount, TalkCount]
            to: [ToFloat, InInt]
          - from: [ToFloat, ReturnValue]
            to: [Multiply, A]
          - from: [GetBonus, DefendBonusPerStack]
            to: [Multiply, B]
          - from: [GetBaseChance, BaseDefendChance]
            to: [Add, A]
          - from: [Multiply, ReturnValue]
            to: [Add, B]
          - from: [Add, ReturnValue]
            to: [Min, A]
          - from: [GetMaxChance, MaxDefendChance]
            to: [Min, B]
          - from: [Min, ReturnValue]
            to: [Return, ReturnValue]

      # ─────────────────────────────────────────────────────────────
      # StopFollowing - Clean up for TIMEOUT (returns to patrol)
      # Removes Following/Bonded/Defending tags, clears timers, unbinds
      # Called from: OnFollowDurationExpired
      # ─────────────────────────────────────────────────────────────
      - name: StopFollowing
        nodes:
          # Clear Talk Timer
          - id: GetTalkTimer
            type: VariableGet
            properties:
              variable_name: TalkCheckTimerHandle
            position: [200, 0]
          - id: ClearTalkTimer
            type: CallFunction
            properties:
              function: K2_ClearAndInvalidateTimerHandle
              class: KismetSystemLibrary
              target_self: true
            position: [400, 0]

          # Clear Attack Timer
          - id: GetAttackTimer
            type: VariableGet
            properties:
              variable_name: AttackCheckTimerHandle
            position: [400, 100]
          - id: ClearAttackTimer
            type: CallFunction
            properties:
              function: K2_ClearAndInvalidateTimerHandle
              class: KismetSystemLibrary
              target_self: true
            position: [600, 0]

          # Clear Duration Timer
          - id: GetDurationTimer
            type: VariableGet
            properties:
              variable_name: FollowDurationTimerHandle
            position: [600, 100]
          - id: ClearDurationTimer
            type: CallFunction
            properties:
              function: K2_ClearAndInvalidateTimerHandle
              class: KismetSystemLibrary
              target_self: true
            position: [800, 0]

          # Check player valid for unbind
          - id: GetCachedPlayer
            type: VariableGet
            properties:
              variable_name: CachedPlayerCharacter
            position: [800, 100]
          - id: IsPlayerValid
            type: CallFunction
            properties:
              function: IsValid
              class: KismetSystemLibrary
            position: [1000, 0]
          - id: BranchPlayerValid
            type: Branch
            position: [1200, 0]

          # Unbind from player damage (True path)
          - id: UnbindDamage
            type: CallFunction
            properties:
              function: UnbindFromOnDamageReceived
              comment: Unbind OnPlayerDamaged from player
            position: [1400, -50]

          # Check follow goal valid
          - id: GetFollowGoal
            type: VariableGet
            properties:
              variable_name: CurrentFollowGoal
            position: [1400, 100]
          - id: IsGoalValid
            type: CallFunction
            properties:
              function: IsValid
              class: KismetSystemLibrary
            position: [1600, 0]
          - id: BranchGoalValid
            type: Branch
            position: [1800, 0]

          # Remove goal (True path)
          - id: RemoveGoal
            type: CallFunction
            properties:
              function: RemoveGoalItem
              comment: Inherited from NPCGoalGenerator
            position: [2000, -50]

          # Clear references
          - id: ClearFollowGoal
            type: VariableSet
            properties:
              variable_name: CurrentFollowGoal
            position: [2200, 0]
          - id: ClearDefendGoal
            type: VariableSet
            properties:
              variable_name: CurrentDefendGoal
            position: [2400, 0]
          - id: ClearPlayer
            type: VariableSet
            properties:
              variable_name: CachedPlayerCharacter
            position: [2600, 0]

          # AUDIT PATCH-C: Remove state tags (Following, Bonded, Defending)
          - id: GetNPCForTags
            type: CallFunction
            properties:
              function: GetControlledPawn
            position: [2800, 0]
          - id: GetASCForTags
            type: CallFunction
            properties:
              function: GetAbilitySystemComponent
            position: [3000, 0]
          - id: RemoveFollowingTag
            type: CallFunction
            properties:
              function: RemoveLooseGameplayTag
              parameters:
                tag: "State.NPC.Returned.Following"
            position: [3200, 0]
          - id: RemoveBondedTag
            type: CallFunction
            properties:
              function: RemoveLooseGameplayTag
              parameters:
                tag: "State.NPC.Returned.Bonded"
            position: [3400, 0]
          - id: RemoveDefendingTag
            type: CallFunction
            properties:
              function: RemoveLooseGameplayTag
              parameters:
                tag: "State.NPC.Returned.Defending"
            position: [3600, 0]

          # Reset state flags
          - id: SetNotFollowing
            type: VariableSet
            properties:
              variable_name: bIsFollowing
              value: false
            position: [3800, 0]
          - id: SetNotDefending
            type: VariableSet
            properties:
              variable_name: bIsDefending
              value: false
            position: [4000, 0]
          - id: ResetTalkCount
            type: VariableSet
            properties:
              variable_name: TalkCount
              value: 0
            position: [4200, 0]

          # Debug print
          - id: PrintStopped
            type: PrintString
            properties:
              message: "GoalGenerator_RandomAggression: Stopped following, returning to patrol"
            position: [4400, 0]

        connections:
          - from: [FunctionEntry, Then]
            to: [ClearTalkTimer, Exec]
          - from: [GetTalkTimer, TalkCheckTimerHandle]
            to: [ClearTalkTimer, Handle]
          - from: [ClearTalkTimer, Then]
            to: [ClearAttackTimer, Exec]
          - from: [GetAttackTimer, AttackCheckTimerHandle]
            to: [ClearAttackTimer, Handle]
          - from: [ClearAttackTimer, Then]
            to: [ClearDurationTimer, Exec]
          - from: [GetDurationTimer, FollowDurationTimerHandle]
            to: [ClearDurationTimer, Handle]
          - from: [ClearDurationTimer, Then]
            to: [IsPlayerValid, Exec]
          - from: [GetCachedPlayer, CachedPlayerCharacter]
            to: [IsPlayerValid, Object]
          - from: [IsPlayerValid, ReturnValue]
            to: [BranchPlayerValid, Condition]
          - from: [IsPlayerValid, Then]
            to: [BranchPlayerValid, Exec]
          - from: [BranchPlayerValid, True]
            to: [UnbindDamage, Exec]
          - from: [GetCachedPlayer, CachedPlayerCharacter]
            to: [UnbindDamage, Target]
          - from: [UnbindDamage, Then]
            to: [IsGoalValid, Exec]
          - from: [BranchPlayerValid, False]
            to: [IsGoalValid, Exec]
          - from: [GetFollowGoal, CurrentFollowGoal]
            to: [IsGoalValid, Object]
          - from: [IsGoalValid, ReturnValue]
            to: [BranchGoalValid, Condition]
          - from: [IsGoalValid, Then]
            to: [BranchGoalValid, Exec]
          - from: [BranchGoalValid, True]
            to: [RemoveGoal, Exec]
          - from: [GetFollowGoal, CurrentFollowGoal]
            to: [RemoveGoal, Goal]
          - from: [RemoveGoal, Then]
            to: [ClearFollowGoal, Exec]
          - from: [BranchGoalValid, False]
            to: [ClearFollowGoal, Exec]
          - from: [ClearFollowGoal, Then]
            to: [ClearDefendGoal, Exec]
          - from: [ClearDefendGoal, Then]
            to: [ClearPlayer, Exec]
          # AUDIT PATCH-C: Tag removal connections
          - from: [ClearPlayer, Then]
            to: [GetNPCForTags, Exec]
          - from: [GetNPCForTags, ReturnValue]
            to: [GetASCForTags, Target]
          - from: [GetNPCForTags, Then]
            to: [GetASCForTags, Exec]
          - from: [GetASCForTags, ReturnValue]
            to: [RemoveFollowingTag, Target]
          - from: [GetASCForTags, Then]
            to: [RemoveFollowingTag, Exec]
          - from: [GetASCForTags, ReturnValue]
            to: [RemoveBondedTag, Target]
          - from: [RemoveFollowingTag, Then]
            to: [RemoveBondedTag, Exec]
          - from: [GetASCForTags, ReturnValue]
            to: [RemoveDefendingTag, Target]
          - from: [RemoveBondedTag, Then]
            to: [RemoveDefendingTag, Exec]
          - from: [RemoveDefendingTag, Then]
            to: [SetNotFollowing, Exec]
          - from: [SetNotFollowing, Then]
            to: [SetNotDefending, Exec]
          - from: [SetNotDefending, Then]
            to: [ResetTalkCount, Exec]
          - from: [ResetTalkCount, Then]
            to: [PrintStopped, Exec]

      # ─────────────────────────────────────────────────────────────
      # BecomeAggressive - AUDIT PATCH-D: Clean up for ATTACK (permanent aggro)
      # Removes Following/Bonded tags, ADDS Aggressive tag, clears timers
      # Does NOT return to patrol - stays hostile permanently
      # Called from: OnAttackCheckTimer (when attack roll succeeds)
      # ─────────────────────────────────────────────────────────────
      - name: BecomeAggressive
        nodes:
          # Clear all looping timers (same as StopFollowing)
          - id: GetTalkTimer
            type: VariableGet
            properties:
              variable_name: TalkCheckTimerHandle
            position: [200, 0]
          - id: ClearTalkTimer
            type: CallFunction
            properties:
              function: K2_ClearAndInvalidateTimerHandle
              class: KismetSystemLibrary
              target_self: true
            position: [400, 0]
          - id: GetAttackTimer
            type: VariableGet
            properties:
              variable_name: AttackCheckTimerHandle
            position: [400, 100]
          - id: ClearAttackTimer
            type: CallFunction
            properties:
              function: K2_ClearAndInvalidateTimerHandle
              class: KismetSystemLibrary
              target_self: true
            position: [600, 0]
          - id: GetDurationTimer
            type: VariableGet
            properties:
              variable_name: FollowDurationTimerHandle
            position: [600, 100]
          - id: ClearDurationTimer
            type: CallFunction
            properties:
              function: K2_ClearAndInvalidateTimerHandle
              class: KismetSystemLibrary
              target_self: true
            position: [800, 0]

          # Unbind from player damage
          - id: GetCachedPlayer
            type: VariableGet
            properties:
              variable_name: CachedPlayerCharacter
            position: [800, 100]
          - id: IsPlayerValid
            type: CallFunction
            properties:
              function: IsValid
              class: KismetSystemLibrary
            position: [1000, 0]
          - id: BranchPlayerValid
            type: Branch
            position: [1200, 0]
          - id: UnbindDamage
            type: CallFunction
            properties:
              function: UnbindFromOnDamageReceived
            position: [1400, -50]

          # Remove follow goal (but keep attack goal active!)
          - id: GetFollowGoal
            type: VariableGet
            properties:
              variable_name: CurrentFollowGoal
            position: [1400, 100]
          - id: IsGoalValid
            type: CallFunction
            properties:
              function: IsValid
              class: KismetSystemLibrary
            position: [1600, 0]
          - id: BranchGoalValid
            type: Branch
            position: [1800, 0]
          - id: RemoveGoal
            type: CallFunction
            properties:
              function: RemoveGoalItem
            position: [2000, -50]

          # Clear references (but NOT defend goal - may still be active)
          - id: ClearFollowGoal
            type: VariableSet
            properties:
              variable_name: CurrentFollowGoal
            position: [2200, 0]
          - id: ClearPlayer
            type: VariableSet
            properties:
              variable_name: CachedPlayerCharacter
            position: [2400, 0]

          # Update state tags: Remove Following/Bonded, ADD Aggressive
          - id: GetNPCForTags
            type: CallFunction
            properties:
              function: GetControlledPawn
            position: [2600, 0]
          - id: GetASCForTags
            type: CallFunction
            properties:
              function: GetAbilitySystemComponent
            position: [2800, 0]
          - id: RemoveFollowingTag
            type: CallFunction
            properties:
              function: RemoveLooseGameplayTag
              parameters:
                tag: "State.NPC.Returned.Following"
            position: [3000, 0]
          - id: RemoveBondedTag
            type: CallFunction
            properties:
              function: RemoveLooseGameplayTag
              parameters:
                tag: "State.NPC.Returned.Bonded"
            position: [3200, 0]
          # NOTE: We do NOT remove Defending tag here - let OnDefendGoalCompleted handle it
          # NOTE: Aggressive tag already added in OnAttackCheckTimer before calling this

          # Reset following state (but stay aggressive)
          - id: SetNotFollowing
            type: VariableSet
            properties:
              variable_name: bIsFollowing
              value: false
            position: [3400, 0]
          # NOTE: bIsDefending NOT reset - defend goal may still be active
          # NOTE: TalkCount NOT reset - irrelevant now that we're aggressive

          # Debug print
          - id: PrintAggressive
            type: PrintString
            properties:
              message: "GoalGenerator_RandomAggression: BECAME PERMANENTLY AGGRESSIVE"
            position: [3600, 0]

        connections:
          - from: [FunctionEntry, Then]
            to: [ClearTalkTimer, Exec]
          - from: [GetTalkTimer, TalkCheckTimerHandle]
            to: [ClearTalkTimer, Handle]
          - from: [ClearTalkTimer, Then]
            to: [ClearAttackTimer, Exec]
          - from: [GetAttackTimer, AttackCheckTimerHandle]
            to: [ClearAttackTimer, Handle]
          - from: [ClearAttackTimer, Then]
            to: [ClearDurationTimer, Exec]
          - from: [GetDurationTimer, FollowDurationTimerHandle]
            to: [ClearDurationTimer, Handle]
          - from: [ClearDurationTimer, Then]
            to: [IsPlayerValid, Exec]
          - from: [GetCachedPlayer, CachedPlayerCharacter]
            to: [IsPlayerValid, Object]
          - from: [IsPlayerValid, ReturnValue]
            to: [BranchPlayerValid, Condition]
          - from: [IsPlayerValid, Then]
            to: [BranchPlayerValid, Exec]
          - from: [BranchPlayerValid, True]
            to: [UnbindDamage, Exec]
          - from: [GetCachedPlayer, CachedPlayerCharacter]
            to: [UnbindDamage, Target]
          - from: [UnbindDamage, Then]
            to: [IsGoalValid, Exec]
          - from: [BranchPlayerValid, False]
            to: [IsGoalValid, Exec]
          - from: [GetFollowGoal, CurrentFollowGoal]
            to: [IsGoalValid, Object]
          - from: [IsGoalValid, ReturnValue]
            to: [BranchGoalValid, Condition]
          - from: [IsGoalValid, Then]
            to: [BranchGoalValid, Exec]
          - from: [BranchGoalValid, True]
            to: [RemoveGoal, Exec]
          - from: [GetFollowGoal, CurrentFollowGoal]
            to: [RemoveGoal, Goal]
          - from: [RemoveGoal, Then]
            to: [ClearFollowGoal, Exec]
          - from: [BranchGoalValid, False]
            to: [ClearFollowGoal, Exec]
          - from: [ClearFollowGoal, Then]
            to: [ClearPlayer, Exec]
          - from: [ClearPlayer, Then]
            to: [GetNPCForTags, Exec]
          - from: [GetNPCForTags, ReturnValue]
            to: [GetASCForTags, Target]
          - from: [GetNPCForTags, Then]
            to: [GetASCForTags, Exec]
          - from: [GetASCForTags, ReturnValue]
            to: [RemoveFollowingTag, Target]
          - from: [GetASCForTags, Then]
            to: [RemoveFollowingTag, Exec]
          - from: [GetASCForTags, ReturnValue]
            to: [RemoveBondedTag, Target]
          - from: [RemoveFollowingTag, Then]
            to: [RemoveBondedTag, Exec]
          - from: [RemoveBondedTag, Then]
            to: [SetNotFollowing, Exec]
          - from: [SetNotFollowing, Then]
            to: [PrintAggressive, Exec]
```

### 3.4 Event Graph - 6 CustomEvents (AUDIT PATCHED)

Due to size, event graph implementation follows the patterns established above. Each event includes:

| Event | Trigger | Key Actions |
|-------|---------|-------------|
| **OnFollowCheckTimer** | Timer (8s loop) | Validate controller → Check !bIsFollowing → Roll FollowChance → Get player → Check range → Cache player → Create Goal_FollowCharacter → Add goal → **ADD Following tag** → Set state → Play dialogue → Bind to damage → Start 3 timers |
| **OnTalkCheckTimer** | Timer (8s loop) | Check bIsFollowing → Check TalkCount < Max → Roll TalkChance → Increment count → **ADD Bonded tag (if TalkCount==1)** → Select dialogue tag → Play dialogue → Extend duration timer |
| **OnPlayerDamaged** | **ASC OnDamagedBy delegate** | Check bIsFollowing & !bIsDefending → **GetAvatarActor from ASC** → Validate attacker → Check range → Roll DefendChance → Create Goal_Attack → **ADD Defending tag** → Set defending → Play dialogue → Bind success+fail |
| **OnDefendGoalCompleted** | Goal success/fail | **REMOVE Defending tag** → Set !bIsDefending → Clear goal ref |
| **OnAttackCheckTimer** | Timer (3s loop) | Check bIsFollowing & !bIsDefending → Roll AttackChance → Create Goal_Attack vs player → **ADD Aggressive tag** → Play dialogue → Add goal → **BecomeAggressive()** (NOT StopFollowing) |
| **OnFollowDurationExpired** | Timer (one-shot) | Play GivingUp dialogue → StopFollowing() |

### OnPlayerDamaged Signature Fix (AUDIT PATCH-B)

**WRONG (v1.0):**
```yaml
- id: Event_OnPlayerDamaged
  type: CustomEvent
  properties:
    event_name: OnPlayerDamaged
    parameters:
      - name: DamageInstigator
        type: Actor
      - name: DamageAmount
        type: Float
```

**CORRECT (v1.0.1):**
```yaml
- id: Event_OnPlayerDamaged
  type: CustomEvent
  properties:
    event_name: OnPlayerDamaged
    parameters:
      - name: DamagerCauserASC
        type: NarrativeAbilitySystemComponent
      - name: Damage
        type: Float
      - name: Spec
        type: GameplayEffectSpec
```

**To get attacker Actor from ASC:**
```yaml
- id: GetAttackerActor
  type: CallFunction
  properties:
    function: GetAvatarActor
    target: DamagerCauserASC  # From event parameter
```

This matches the Narrative Pro delegate signature:
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamagedBy,
    UNarrativeAbilitySystemComponent*, DamagerCauserASC,
    const float, Damage,
    const FGameplayEffectSpec&, Spec);
```

---

## PART 4: BP_RETURNEDSTAKLER (FULL SETUP)

```yaml
actor_blueprints:  # STALKER - NPC BLUEPRINT
  - name: BP_ReturnedStalker
    folder: NPCs/Returned
    parent_class: NarrativeNPCCharacter
    variables:
      # Visual feedback for bond level
      - name: BondLevelMaterial
        type: Object
        class: MaterialInstanceDynamic
        comment: MID for visual bond feedback
      # Original material for reset
      - name: OriginalMaterial
        type: Object
        class: MaterialInterface

    event_graph:
      nodes:
        # BeginPlay - Initialize visual systems
        - id: Event_BeginPlay
          type: Event
          properties:
            event_name: ReceiveBeginPlay
          position: [0, 0]

        # Call parent BeginPlay
        - id: CallParentBeginPlay
          type: CallParent
          position: [200, 0]

        # Store original material for reset
        - id: GetMesh
          type: CallFunction
          properties:
            function: GetMesh
          position: [400, 0]
        - id: GetMaterial
          type: CallFunction
          properties:
            function: GetMaterial
            parameters:
              ElementIndex: 0
          position: [600, 0]
        - id: SetOriginalMaterial
          type: VariableSet
          properties:
            variable_name: OriginalMaterial
          position: [800, 0]

        # Create dynamic material instance for bond feedback
        - id: CreateDMI
          type: CallFunction
          properties:
            function: CreateDynamicMaterialInstance
            parameters:
              ElementIndex: 0
          position: [1000, 0]
        - id: SetBondMaterial
          type: VariableSet
          properties:
            variable_name: BondLevelMaterial
          position: [1200, 0]

        # Apply Returned faction tag
        - id: GetASC
          type: CallFunction
          properties:
            function: GetAbilitySystemComponent
          position: [1400, 0]
        - id: AddFactionTag
          type: CallFunction
          properties:
            function: AddLooseGameplayTag
            parameters:
              tag: "Faction.NPC.Returned"
          position: [1600, 0]

        # Debug print
        - id: PrintReady
          type: PrintString
          properties:
            message: "BP_ReturnedStalker: Initialized with bond visual system"
          position: [1800, 0]

      connections:
        - from: [Event_BeginPlay, Then]
          to: [CallParentBeginPlay, Exec]
        - from: [CallParentBeginPlay, Exec]
          to: [GetMesh, Exec]
        - from: [GetMesh, ReturnValue]
          to: [GetMaterial, Target]
        - from: [GetMaterial, Then]
          to: [SetOriginalMaterial, Exec]
        - from: [GetMaterial, ReturnValue]
          to: [SetOriginalMaterial, OriginalMaterial]
        - from: [SetOriginalMaterial, Then]
          to: [CreateDMI, Exec]
        - from: [GetMesh, ReturnValue]
          to: [CreateDMI, Target]
        - from: [CreateDMI, ReturnValue]
          to: [SetBondMaterial, BondLevelMaterial]
        - from: [CreateDMI, Then]
          to: [SetBondMaterial, Exec]
        - from: [SetBondMaterial, Then]
          to: [GetASC, Exec]
        - from: [GetASC, ReturnValue]
          to: [AddFactionTag, Target]
        - from: [AddFactionTag, Then]
          to: [PrintReady, Exec]
```

---

## PART 5: TAGGED DIALOGUE SET

```yaml
tagged_dialogue_sets:
  - name: TaggedDialogueSet_Returned
    folder: Dialogue/TaggedSets
    dialogues:
      - tag: Narrative.TaggedDialogue.Returned.StartFollowing
        dialogue: DBP_Returned_StartFollow
        cooldown: 30.0
        max_distance: 5000.0
      - tag: Narrative.TaggedDialogue.Returned.Following.First
        dialogue: DBP_Returned_First
        cooldown: 30.0
        max_distance: 5000.0
      - tag: Narrative.TaggedDialogue.Returned.Following.Bonding
        dialogue: DBP_Returned_Bonding
        cooldown: 30.0
        max_distance: 5000.0
      - tag: Narrative.TaggedDialogue.Returned.Following.Deep
        dialogue: DBP_Returned_Deep
        cooldown: 30.0
        max_distance: 5000.0
      - tag: Narrative.TaggedDialogue.Returned.Defending
        dialogue: DBP_Returned_Defending
        cooldown: 5.0
        max_distance: 5000.0
      - tag: Narrative.TaggedDialogue.Returned.Attacking
        dialogue: DBP_Returned_Attacking
        cooldown: 5.0
        max_distance: 5000.0
      - tag: Narrative.TaggedDialogue.Returned.GivingUp
        dialogue: DBP_Returned_GivingUp
        cooldown: 30.0
        max_distance: 5000.0
```

---

## PART 6: DIALOGUE BLUEPRINTS (7 Placeholder)

```yaml
dialogue_blueprints:
  - name: DBP_Returned_StartFollow
    folder: Dialogue/Returned
    parent_class: NarrativeDialogue
    free_movement: true
    speakers:
      - npc_definition: NPC_ReturnedStalker
    dialogue_tree:
      root: start_node
      nodes:
        - id: start_node
          type: npc
          speaker: NPC_ReturnedStalker
          text: "*The creature tilts its head and begins following you*"
          duration: 2.0

  - name: DBP_Returned_First
    folder: Dialogue/Returned
    parent_class: NarrativeDialogue
    free_movement: true
    speakers:
      - npc_definition: NPC_ReturnedStalker
    dialogue_tree:
      root: first_node
      nodes:
        - id: first_node
          type: npc
          speaker: NPC_ReturnedStalker
          text: "*The creature makes a curious chirping sound*"
          duration: 2.0

  - name: DBP_Returned_Bonding
    folder: Dialogue/Returned
    parent_class: NarrativeDialogue
    free_movement: true
    speakers:
      - npc_definition: NPC_ReturnedStalker
    dialogue_tree:
      root: bonding_node
      nodes:
        - id: bonding_node
          type: npc
          speaker: NPC_ReturnedStalker
          text: "*The creature nuzzles closer, seeming more comfortable*"
          duration: 2.0

  - name: DBP_Returned_Deep
    folder: Dialogue/Returned
    parent_class: NarrativeDialogue
    free_movement: true
    speakers:
      - npc_definition: NPC_ReturnedStalker
    dialogue_tree:
      root: deep_node
      nodes:
        - id: deep_node
          type: npc
          speaker: NPC_ReturnedStalker
          text: "*The creature purrs contentedly, fully bonded*"
          duration: 2.0

  - name: DBP_Returned_Defending
    folder: Dialogue/Returned
    parent_class: NarrativeDialogue
    free_movement: true
    speakers:
      - npc_definition: NPC_ReturnedStalker
    dialogue_tree:
      root: defending_node
      nodes:
        - id: defending_node
          type: npc
          speaker: NPC_ReturnedStalker
          text: "*The creature SCREECHES and charges at your attacker!*"
          duration: 1.5

  - name: DBP_Returned_Attacking
    folder: Dialogue/Returned
    parent_class: NarrativeDialogue
    free_movement: true
    speakers:
      - npc_definition: NPC_ReturnedStalker
    dialogue_tree:
      root: attacking_node
      nodes:
        - id: attacking_node
          type: npc
          speaker: NPC_ReturnedStalker
          text: "*The creature's eyes turn RED as it lunges at YOU!*"
          duration: 1.5

  - name: DBP_Returned_GivingUp
    folder: Dialogue/Returned
    parent_class: NarrativeDialogue
    free_movement: true
    speakers:
      - npc_definition: NPC_ReturnedStalker
    dialogue_tree:
      root: givingup_node
      nodes:
        - id: givingup_node
          type: npc
          speaker: NPC_ReturnedStalker
          text: "*The creature loses interest and wanders away*"
          duration: 2.0
```

---

## PART 7: NPC DEFINITION UPDATE

```yaml
npc_definitions:
  - name: NPC_ReturnedStalker
    folder: NPCs/Returned
    npc_id: ReturnedStalker
    npc_name: Returned
    npc_blueprint: BP_ReturnedStalker
    ability_configuration: AC_ReturnedStalker
    activity_configuration: AC_ReturnedStalkerBehavior
    tagged_dialogue_set: TaggedDialogueSet_Returned
    default_factions:
      - Faction.NPC.Returned
    default_owned_tags:
      - State.NPC.ReturnedStalker
```

---

## IMPLEMENTATION CHECKLIST (AUDIT PATCHED)

| # | Component | Type | Status |
|---|-----------|------|--------|
| 1 | Faction.NPC.Returned | Tag | ☐ |
| 2 | GoalGenerator_RandomAggression.variables (35) | Variables | ☐ |
| 3 | GoalGenerator_RandomAggression.function_overrides | Override | ☐ |
| 4 | GoalGenerator_RandomAggression.functions (**5**) | Functions | ☐ |
| 4a | - GetCurrentFollowDuration | Function | ☐ |
| 4b | - GetCurrentAttackChance | Function | ☐ |
| 4c | - GetCurrentDefendChance | Function | ☐ |
| 4d | - StopFollowing (with state tags) | Function | ☐ |
| 4e | - **BecomeAggressive** (PATCH-D) | Function | ☐ |
| 5 | GoalGenerator_RandomAggression.event_graph | Events (6) | ☐ |
| 5a | - OnFollowCheckTimer (+Following tag) | Event | ☐ |
| 5b | - OnTalkCheckTimer (+Bonded tag) | Event | ☐ |
| 5c | - **OnPlayerDamaged (FIXED signature)** | Event | ☐ |
| 5d | - OnDefendGoalCompleted (-Defending tag) | Event | ☐ |
| 5e | - OnAttackCheckTimer (+Aggressive, BecomeAggressive) | Event | ☐ |
| 5f | - OnFollowDurationExpired | Event | ☐ |
| 6 | BP_ReturnedStalker.variables | Variables | ☐ |
| 7 | BP_ReturnedStalker.event_graph | BeginPlay | ☐ |
| 8 | TaggedDialogueSet_Returned | DataAsset | ☐ |
| 9 | DBP_Returned_StartFollow | Dialogue | ☐ |
| 10 | DBP_Returned_First | Dialogue | ☐ |
| 11 | DBP_Returned_Bonding | Dialogue | ☐ |
| 12 | DBP_Returned_Deep | Dialogue | ☐ |
| 13 | DBP_Returned_Defending | Dialogue | ☐ |
| 14 | DBP_Returned_Attacking | Dialogue | ☐ |
| 15 | DBP_Returned_GivingUp | Dialogue | ☐ |
| 16 | NPC_ReturnedStalker | Update | ☐ |

---

## ESTIMATED METRICS

| Metric | Value |
|--------|-------|
| Total Variables | 37 (35 GoalGen + 2 BP) |
| Total Functions | **5** (GetCurrentFollowDuration, GetCurrentAttackChance, GetCurrentDefendChance, StopFollowing, **BecomeAggressive**) |
| Total CustomEvents | 6 |
| Total Event Graph Nodes | ~200 |
| Total Connections | ~280 |
| New Assets | 9 |
| Modified Assets | 2 |

---

## CONTRACTS COMPLIANCE (AUDIT PATCHED)

| Contract | Requirement | Compliance |
|----------|-------------|------------|
| R-TIMER-2 | Validity guard in callbacks | All timer events check OwnerController validity first |
| R-DELEGATE-1 | Signature match | **FIXED:** OnPlayerDamaged(ASC, Float, Spec) matches FOnDamagedBy |
| R-STATE-TAGS-1 | State tag consistency | **ADDED:** Following/Bonded/Defending/Aggressive applied/removed per state transitions |
| R-TIMER-FSM-1 | Timer hygiene | StopFollowing and BecomeAggressive both clear all looping timers |

---

## RISK MITIGATIONS (AUDIT PATCHED)

| Risk | Mitigation |
|------|------------|
| ~~Delegate signature mismatch~~ | **RESOLVED:** Signature corrected to match FOnDamagedBy (ASC, Float, Spec) |
| Timer leak on NPC death | All callbacks check validity, StopFollowing/BecomeAggressive clear all handles |
| Goal reference leak | Both cleanup functions clear goal refs appropriately |
| Defend stuck state | Bind to BOTH OnGoalSucceeded AND OnGoalFailed |
| **Aggressive state reset** | **RESOLVED:** BecomeAggressive() preserves Aggressive tag, doesn't return to patrol |

---

## AUDIT RESOLUTION SUMMARY

| GPT Finding | Status | Resolution |
|-------------|--------|------------|
| A) Timer handles only 3 | **REJECTED** | All 4 already present (lines 208-218) |
| B) Delegate signature wrong | **ACCEPTED** | Fixed to FOnDamagedBy(ASC, Float, Spec) |
| C) Missing state tag application | **ACCEPTED** | Added State Tag Contract (Part 2.5) + tag nodes in cleanup functions |
| D) StopFollowing breaks Aggressive | **ACCEPTED** | Added BecomeAggressive() function for attack path |

---

**STATUS: COMPREHENSIVE PLAN v1.0.1 - AUDIT PATCHED - READY FOR APPROVAL**

**Next:** User approves → Implementation begins
