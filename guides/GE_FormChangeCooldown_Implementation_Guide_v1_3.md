# Father Companion - Form Change Cooldown Implementation Guide
## VERSION 1.3 - Simplified Tag Creation
## Unreal Engine 5.6 + Narrative Pro Plugin v2.2

---

## **DOCUMENT INFORMATION**

| Field | Value |
|-------|-------|
| Document Type | Implementation Guide |
| Effect Name | GE_FormChangeCooldown |
| Parent Class | GameplayEffect |
| Version | 1.3 |
| Last Updated | January 2026 |
| Estimated Time | 15 minutes |

---

## **PREREQUISITES**

| Requirement | Description |
|-------------|-------------|
| ReplicateActivationOwnedTags | ENABLED in Project Settings -> Gameplay Abilities Settings |
| Cooldown.Father.FormChange tag | Must exist in Gameplay Tags |

---

## **PHASE 1: CREATE COOLDOWN TAG**

### **Create Required Tags**

| Tag Name | Purpose |
|----------|---------|
| Cooldown.Father.FormChange | Form change cooldown tag - blocks rapid switching |

---

## **PHASE 2: CREATE GE_FORMCHANGECOOLDOWN**

### **1) Create Gameplay Effect Asset**

#### 1.1) Create New GameplayEffect
   - 1.1.1) Right-click in /Content/FatherCompanion/Effects/
   - 1.1.2) Select Blueprint Class
   - 1.1.3) In All Classes search, type GameplayEffect
   - 1.1.4) Select GameplayEffect as parent class
   - 1.1.5) Name: GE_FormChangeCooldown
   - 1.1.6) Press Enter
   - 1.1.7) Double-click GE_FormChangeCooldown to open

### **2) Configure Duration Policy**

#### 2.1) Set Has Duration
   - 2.1.1) Click Class Defaults button in toolbar
   - 2.1.2) In Details panel, locate Gameplay Effect section
   - 2.1.3) Find Duration Policy dropdown
   - 2.1.4) Select: Has Duration

#### 2.2) Set Duration Magnitude
   - 2.2.1) Locate Duration Magnitude section
   - 2.2.2) Expand Scalable Float Magnitude
   - 2.2.3) Set Value: 15.0

### **3) Add Grant Tags Component**

#### 3.1) Access Components Array
   - 3.1.1) In Details panel, locate Components section
   - 3.1.2) Click + (Plus) button to add component

#### 3.2) Add Grant Tags to Target Actor
   - 3.2.1) In component dropdown, select: **Grant Tags to Target Actor**

#### 3.3) Configure Granted Tags
   - 3.3.1) Expand the newly added component in Components array
   - 3.3.2) Expand **Add Tags** subsection
   - 3.3.3) In **Add to Inherited** array, click + button
   - 3.3.4) Select tag: `Cooldown.Father.FormChange`

### **4) Add Asset Tags Component**

#### 4.1) Add Component for Effect Identification
   - 4.1.1) In Components section, click + (Plus) button
   - 4.1.2) In component dropdown, select: **Tags This Effect Has (Asset Tags)**

#### 4.2) Configure Asset Tags
   - 4.2.1) Expand the newly added component in Components array
   - 4.2.2) Expand **Add Tags** subsection
   - 4.2.3) In **Add to Inherited** array, click + button
   - 4.2.4) Select tag: `Cooldown.Father.FormChange`

### **5) Compile and Save**

#### 5.1) Finalize Effect
   - 5.1.1) Click Compile button in toolbar
   - 5.1.2) Click Save button in toolbar

---

## **PHASE 3: CONFIGURE FORM ABILITIES (Built-in Cooldown System)**

### **1) Set CooldownGameplayEffectClass Property**

GAS automatically blocks ability activation when cooldown tag is present.

#### 1.1) For Each Form Ability (GA_FatherCrawler, GA_FatherArmor, GA_FatherExoskeleton, GA_FatherSymbiote, GA_FatherEngineer)
   - 1.1.1) Open ability blueprint
   - 1.1.2) Click Class Defaults
   - 1.1.3) Find **Cooldown** section
   - 1.1.4) Set **Cooldown Gameplay Effect Class**: `GE_FormChangeCooldown`

### **2) Apply Cooldown Using CommitAbilityCooldown**

Each form ability calls CommitAbilityCooldown after successful form change.

#### 2.1) Replace Manual Apply Effect Node
   - 2.1.1) In Event Graph, after form change logic completes
   - 2.1.2) Remove any existing `Apply Gameplay Effect to Self` node for cooldown
   - 2.1.3) Add **Commit Ability Cooldown** node
   - 2.1.4) No parameters needed - uses CooldownGameplayEffectClass automatically

### **3) Remove Block Abilities with Tag (No Longer Needed)**

GAS automatically checks for cooldown tag presence before allowing activation.

#### 3.1) For Each Form Ability
   - 3.1.1) Open ability blueprint
   - 3.1.2) Click Class Defaults
   - 3.1.3) Find **Block Abilities with Tag** property
   - 3.1.4) Remove: `Cooldown.Father.FormChange` (if present)

---

## **QUICK REFERENCE**

### **Effect Configuration**

| Property | Value |
|----------|-------|
| Duration Policy | Has Duration |
| Duration Magnitude | 15.0 seconds |
| Granted Tag | Cooldown.Father.FormChange |
| Asset Tag | Cooldown.Father.FormChange |

### **Form Ability Integration (Built-in System)**

| Property | Location | Value |
|----------|----------|-------|
| Cooldown Gameplay Effect Class | Class Defaults -> Cooldown | GE_FormChangeCooldown |
| Commit Cooldown | Event Graph | CommitAbilityCooldown node after form change |
| Block Abilities with Tag | Class Defaults | NOT NEEDED - GAS handles automatically |

### **Built-in Cooldown Benefits**

| Benefit | Description |
|---------|-------------|
| Automatic Blocking | GAS checks cooldown tag before activation |
| GetCooldownTimeRemaining() | Built-in function for UI cooldown display |
| Cleaner Architecture | No manual tag blocking configuration |

---

## **RELATED DOCUMENTS**

| Document | Purpose |
|----------|---------|
| GA_FatherArmor_Implementation_Guide_v3_8.md | Armor form ability |
| GA_FatherCrawler_Implementation_Guide_v3_1.md | Crawler form ability |
| GA_FatherExoskeleton_Implementation_Guide_v3_6.md | Exoskeleton form ability |
| GA_FatherSymbiote_Implementation_Guide_v3_2.md | Symbiote form ability |
| GA_FatherEngineer_Implementation_Guide_v3_9.md | Engineer form ability |

---

## **CHANGELOG**

| Version | Date | Changes |
|---------|------|---------|
| 1.3 | January 2026 | Simplified PHASE 1: Replaced detailed tag navigation with consolidated tag table. Reduced PHASE 1 from 18 lines to 6 lines. |
| 1.2 | January 2026 | Built-in cooldown system: CooldownGameplayEffectClass property + CommitAbilityCooldown node. Removed Block Abilities with Tag (GAS handles automatically). Added GetCooldownTimeRemaining() benefit. |
| 1.1 | January 2026 | Fixed GE tag property: Changed "Combined Tags" to "Add to Inherited" per UE 5.6 GE component architecture. |
| 1.0 | January 2026 | Initial guide. Manual cooldown application with Block Abilities with Tag. |

---

**END OF IMPLEMENTATION GUIDE VERSION 1.3**

**Blueprint-Only Implementation for Unreal Engine 5.6 + Narrative Pro Plugin v2.2**
