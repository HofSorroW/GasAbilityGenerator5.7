// GasAbilityGenerator - Dialogue Token Registry
// v4.4: Validated token system for Excel dialogue authoring
//
// Provides bidirectional mapping between token strings and UE event/condition objects.
// Safety principle: Invalid tokens NEVER wipe UE data - they preserve and flag errors.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

// Forward declarations
class UNarrativeEvent;
class UNarrativeCondition;

/**
 * Parameter types for token validation
 */
UENUM()
enum class ETokenParamType : uint8
{
	String,		// Any text value
	Bool,		// true/false
	Int,		// Integer value
	Float,		// Decimal value
	Enum,		// Must match enum values
	IdRef		// Must exist in _Lists (validated asset reference)
};

/**
 * Token categories
 */
UENUM()
enum class ETokenCategory : uint8
{
	Event,
	Condition
};

/**
 * Single parameter definition for a token
 */
struct GASABILITYGENERATOR_API FDialogueTokenParam
{
	FName ParamName;			// "QuestId", "ItemId", "Count", etc.
	ETokenParamType Type;		// Validation type
	bool bRequired;				// Must be present in token
	FName UEPropertyName;		// Maps to UObject property name
	FString DefaultValue;		// Default if not specified
	TArray<FString> EnumValues;	// Valid values for Enum type

	FDialogueTokenParam()
		: Type(ETokenParamType::String)
		, bRequired(true)
	{}

	FDialogueTokenParam(FName InName, ETokenParamType InType, bool bInRequired, FName InUEProperty, const FString& InDefault = TEXT(""))
		: ParamName(InName)
		, Type(InType)
		, bRequired(bInRequired)
		, UEPropertyName(InUEProperty)
		, DefaultValue(InDefault)
	{}
};

/**
 * Parsed token data (intermediate representation)
 */
struct GASABILITYGENERATOR_API FParsedToken
{
	FString TokenName;						// "NE_BeginQuest"
	TMap<FString, FString> Params;			// Key=Value pairs
	bool bIsClear = false;					// Special CLEAR token
	FString RawString;						// Original string for error messages
};

/**
 * Token specification - defines how to serialize/deserialize a specific token type
 */
struct GASABILITYGENERATOR_API FDialogueTokenSpec
{
	FString TokenName;						// "NE_BeginQuest", "NC_QuestState", etc.
	ETokenCategory Category;				// Event or Condition
	FString UEClassName;					// "NE_BeginQuest", "BPE_AddItemToInventory", etc.
	FString UEClassPath;					// Full path to Blueprint class
	TArray<FDialogueTokenParam> Params;		// Parameter definitions
	FString DisplayName;					// Human-readable name for UI

	FDialogueTokenSpec()
		: Category(ETokenCategory::Event)
	{}

	// Check if this spec matches a UObject class
	bool MatchesClass(UClass* Class) const;
};

/**
 * Result of token parsing/validation
 */
struct GASABILITYGENERATOR_API FTokenParseResult
{
	bool bSuccess = false;
	FString ErrorMessage;
	TArray<FParsedToken> Tokens;
};

/**
 * Result of token deserialization (tokens -> UObjects)
 */
struct GASABILITYGENERATOR_API FTokenDeserializeResult
{
	bool bSuccess = false;
	FString ErrorMessage;
	TArray<UObject*> Objects;				// Created event/condition objects
	bool bShouldClear = false;				// CLEAR token was present
};

/**
 * Dialogue Token Registry - Singleton registry for token specifications
 *
 * Provides:
 * - Forward mapping: Token string -> UObject (deserialization)
 * - Reverse mapping: UObject -> Token string (serialization)
 * - Validation against known IDs
 * - UNSUPPORTED(...) fallback for unknown types
 */
class GASABILITYGENERATOR_API FDialogueTokenRegistry
{
public:
	/** v4.5: Spec version - bump when token specs change (affects validation staleness) */
	static constexpr uint32 SpecVersion = 1;

	static FDialogueTokenRegistry& Get();

	// Registration
	void RegisterTokenSpec(const FDialogueTokenSpec& Spec);
	void RegisterBuiltInTokens();

	// Lookup
	const FDialogueTokenSpec* FindByTokenName(const FString& TokenName) const;
	const FDialogueTokenSpec* FindByClass(UClass* Class) const;
	const FDialogueTokenSpec* FindByClassName(const FString& ClassName) const;

	// Parsing (string -> structured data)
	FTokenParseResult ParseTokenString(const FString& TokenStr) const;
	FParsedToken ParseSingleToken(const FString& TokenStr, FString& OutError) const;

	// Validation
	bool ValidateToken(const FParsedToken& Token, const FDialogueTokenSpec& Spec, FString& OutError) const;
	bool ValidateIdRef(const FString& IdType, const FString& IdValue, const TSet<FString>& ValidIds) const;

	// Serialization (UObject -> token string)
	FString SerializeEvent(UObject* Event) const;
	FString SerializeCondition(UObject* Condition) const;
	FString SerializeEvents(const TArray<UObject*>& Events) const;
	FString SerializeConditions(const TArray<UObject*>& Conditions) const;

	// Deserialization (token string -> UObjects)
	FTokenDeserializeResult DeserializeEvents(const FString& TokenStr, UObject* Outer) const;
	FTokenDeserializeResult DeserializeConditions(const FString& TokenStr, UObject* Outer) const;

	// ID Lists management (for validation)
	void SetValidIds(const FString& IdType, const TSet<FString>& Ids);
	const TSet<FString>& GetValidIds(const FString& IdType) const;
	void ClearValidIds();

	// Get all registered specs
	const TArray<FDialogueTokenSpec>& GetAllSpecs() const { return Specs; }

private:
	FDialogueTokenRegistry();
	~FDialogueTokenRegistry() = default;

	// Non-copyable
	FDialogueTokenRegistry(const FDialogueTokenRegistry&) = delete;
	FDialogueTokenRegistry& operator=(const FDialogueTokenRegistry&) = delete;

	TArray<FDialogueTokenSpec> Specs;
	TMap<FString, TSet<FString>> ValidIdSets;	// IdType -> Valid IDs

	static TSet<FString> EmptySet;

	// Internal helpers
	UObject* InstantiateFromSpec(const FDialogueTokenSpec& Spec, const FParsedToken& Token, UObject* Outer, FString& OutError) const;
	bool SetPropertyFromParam(UObject* Object, const FDialogueTokenParam& ParamDef, const FString& Value, FString& OutError) const;
	FString GetPropertyAsString(UObject* Object, const FDialogueTokenParam& ParamDef) const;
};

/**
 * Helper macros for token registration
 */
#define REGISTER_EVENT_TOKEN(TokenName, UEClass, DisplayName) \
	{ FDialogueTokenSpec Spec; \
	  Spec.TokenName = TEXT(#TokenName); \
	  Spec.Category = ETokenCategory::Event; \
	  Spec.UEClassName = TEXT(#UEClass); \
	  Spec.DisplayName = TEXT(DisplayName);

#define REGISTER_CONDITION_TOKEN(TokenName, UEClass, DisplayName) \
	{ FDialogueTokenSpec Spec; \
	  Spec.TokenName = TEXT(#TokenName); \
	  Spec.Category = ETokenCategory::Condition; \
	  Spec.UEClassName = TEXT(#UEClass); \
	  Spec.DisplayName = TEXT(DisplayName);

#define ADD_PARAM(Name, Type, Required, UEProp, Default) \
	  Spec.Params.Add(FDialogueTokenParam(TEXT(#Name), ETokenParamType::Type, Required, TEXT(#UEProp), TEXT(Default)));

#define ADD_ENUM_PARAM(Name, UEProp, Values) \
	  { FDialogueTokenParam P(TEXT(#Name), ETokenParamType::Enum, true, TEXT(#UEProp), TEXT("")); \
	    P.EnumValues = Values; \
	    Spec.Params.Add(P); }

#define END_TOKEN() \
	  RegisterTokenSpec(Spec); }
