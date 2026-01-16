// Copyright 2026 GasAbilityGenerator Plugin. All Rights Reserved.

#include "GasAbilityGeneratorDialogueCSVParser.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogDialogueCSVParser, Log, All);

bool FDialogueCSVParser::ParseCSVFile(const FString& CSVFilePath, TArray<FManifestDialogueBlueprintDefinition>& OutDialogues)
{
	FString CSVContent;
	if (!FFileHelper::LoadFileToString(CSVContent, *CSVFilePath))
	{
		UE_LOG(LogDialogueCSVParser, Error, TEXT("Failed to load CSV file: %s"), *CSVFilePath);
		return false;
	}

	UE_LOG(LogDialogueCSVParser, Log, TEXT("Loaded CSV file: %s (%d characters)"), *CSVFilePath, CSVContent.Len());
	return ParseCSVContent(CSVContent, OutDialogues);
}

bool FDialogueCSVParser::ParseCSVContent(const FString& CSVContent, TArray<FManifestDialogueBlueprintDefinition>& OutDialogues)
{
	TArray<FString> Lines;
	CSVContent.ParseIntoArrayLines(Lines, false);

	if (Lines.Num() < 2)
	{
		UE_LOG(LogDialogueCSVParser, Error, TEXT("CSV must have at least a header and one data row"));
		return false;
	}

	// Skip header row
	TArray<FDialogueCSVRow> AllRows;

	for (int32 i = 1; i < Lines.Num(); i++)
	{
		const FString& Line = Lines[i].TrimStartAndEnd();

		// Skip empty lines and comments
		if (Line.IsEmpty() || Line.StartsWith(TEXT("#")))
		{
			continue;
		}

		FDialogueCSVRow Row;
		if (ParseCSVLine(Line, Row) && Row.IsValid())
		{
			AllRows.Add(Row);
		}
		else if (!Line.IsEmpty())
		{
			UE_LOG(LogDialogueCSVParser, Warning, TEXT("Skipping invalid row at line %d: %s"), i + 1, *Line);
		}
	}

	UE_LOG(LogDialogueCSVParser, Log, TEXT("Parsed %d valid rows from CSV"), AllRows.Num());

	// Group by dialogue
	TMap<FString, FParsedDialogueData> GroupedData = GroupByDialogue(AllRows);

	// Convert each dialogue to manifest definition
	for (auto& Pair : GroupedData)
	{
		const FString& DialogueName = Pair.Key;
		FParsedDialogueData& Data = Pair.Value;

		// Validate structure
		TArray<FString> ValidationErrors;
		if (!ValidateDialogueStructure(Data, ValidationErrors))
		{
			for (const FString& Error : ValidationErrors)
			{
				UE_LOG(LogDialogueCSVParser, Error, TEXT("[%s] %s"), *DialogueName, *Error);
			}
			continue;
		}

		// Convert to manifest definition
		FManifestDialogueBlueprintDefinition Definition;
		if (ConvertToManifestDefinition(Data, Definition))
		{
			OutDialogues.Add(Definition);
			UE_LOG(LogDialogueCSVParser, Log, TEXT("Successfully parsed dialogue: %s (%d nodes)"),
			       *DialogueName, Data.Nodes.Num());
		}
	}

	return OutDialogues.Num() > 0;
}

bool FDialogueCSVParser::ParseCSVLine(const FString& Line, FDialogueCSVRow& OutRow)
{
	TArray<FString> Values = SplitCSVLine(Line);

	if (Values.Num() < 7)
	{
		return false;
	}

	OutRow.Dialogue = ParseCSVValue(Values[0]);
	OutRow.NodeID = ParseCSVValue(Values[1]);
	OutRow.Type = ParseCSVValue(Values[2]);
	OutRow.Speaker = Values.Num() > 3 ? ParseCSVValue(Values[3]) : TEXT("");
	OutRow.Text = Values.Num() > 4 ? ParseCSVValue(Values[4]) : TEXT("");
	OutRow.OptionText = Values.Num() > 5 ? ParseCSVValue(Values[5]) : TEXT("");
	OutRow.Replies = Values.Num() > 6 ? ParseCSVValue(Values[6]) : TEXT("");
	OutRow.Conditions = Values.Num() > 7 ? ParseCSVValue(Values[7]) : TEXT("");
	OutRow.Events = Values.Num() > 8 ? ParseCSVValue(Values[8]) : TEXT("");

	return OutRow.IsValid();
}

FString FDialogueCSVParser::ParseCSVValue(const FString& Value)
{
	FString Result = Value.TrimStartAndEnd();

	// Remove surrounding quotes
	if (Result.Len() >= 2 && Result.StartsWith(TEXT("\"")) && Result.EndsWith(TEXT("\"")))
	{
		Result = Result.Mid(1, Result.Len() - 2);
	}

	// Handle escaped quotes
	Result = Result.Replace(TEXT("\"\""), TEXT("\""));

	return Result;
}

TArray<FString> FDialogueCSVParser::SplitCSVLine(const FString& Line)
{
	TArray<FString> Result;
	FString Current;
	bool bInQuotes = false;

	for (int32 i = 0; i < Line.Len(); i++)
	{
		TCHAR C = Line[i];

		if (C == TEXT('"'))
		{
			// Check for escaped quote
			if (bInQuotes && i + 1 < Line.Len() && Line[i + 1] == TEXT('"'))
			{
				Current += TEXT('"');
				i++; // Skip next quote
			}
			else
			{
				bInQuotes = !bInQuotes;
				Current += C;
			}
		}
		else if (C == TEXT(',') && !bInQuotes)
		{
			Result.Add(Current);
			Current.Empty();
		}
		else
		{
			Current += C;
		}
	}

	// Add last value
	Result.Add(Current);

	return Result;
}

TMap<FString, FParsedDialogueData> FDialogueCSVParser::GroupByDialogue(const TArray<FDialogueCSVRow>& Rows)
{
	TMap<FString, FParsedDialogueData> Result;

	for (const FDialogueCSVRow& Row : Rows)
	{
		FParsedDialogueData& Data = Result.FindOrAdd(Row.Dialogue);

		if (Data.DialogueName.IsEmpty())
		{
			Data.DialogueName = Row.Dialogue;
			Data.RootNodeID = Row.NodeID; // First node is root
		}

		Data.Nodes.Add(Row);

		if (!Row.Speaker.IsEmpty())
		{
			Data.Speakers.Add(Row.Speaker);
		}
	}

	return Result;
}

bool FDialogueCSVParser::ValidateDialogueStructure(const FParsedDialogueData& Data, TArray<FString>& OutErrors)
{
	bool bValid = true;

	// Check for duplicate node IDs
	TSet<FString> SeenIDs;
	for (const FDialogueCSVRow& Node : Data.Nodes)
	{
		if (SeenIDs.Contains(Node.NodeID))
		{
			OutErrors.Add(FString::Printf(TEXT("Duplicate NodeID: %s"), *Node.NodeID));
			bValid = false;
		}
		SeenIDs.Add(Node.NodeID);
	}

	// Check that all reply targets exist
	for (const FDialogueCSVRow& Node : Data.Nodes)
	{
		if (Node.IsTerminal())
		{
			continue;
		}

		TArray<FString> ReplyIDs;
		Node.Replies.ParseIntoArray(ReplyIDs, TEXT(";"), true);

		for (const FString& ReplyID : ReplyIDs)
		{
			FString CleanID = ReplyID.TrimStartAndEnd();
			if (!CleanID.Equals(TEXT("END"), ESearchCase::IgnoreCase) &&
			    !CleanID.Equals(TEXT("BACK"), ESearchCase::IgnoreCase) &&
			    !SeenIDs.Contains(CleanID))
			{
				OutErrors.Add(FString::Printf(TEXT("Node '%s' references non-existent reply target: %s"),
				              *Node.NodeID, *CleanID));
				bValid = false;
			}
		}
	}

	// Check NPC nodes have speakers
	for (const FDialogueCSVRow& Node : Data.Nodes)
	{
		if (Node.IsNPCNode() && Node.Speaker.IsEmpty())
		{
			OutErrors.Add(FString::Printf(TEXT("NPC node '%s' has no speaker defined"), *Node.NodeID));
			bValid = false;
		}
	}

	return bValid;
}

bool FDialogueCSVParser::ConvertToManifestDefinition(const FParsedDialogueData& ParsedData, FManifestDialogueBlueprintDefinition& OutDefinition)
{
	OutDefinition.Name = ParsedData.DialogueName;
	OutDefinition.ParentClass = TEXT("NarrativeDialogue");
	OutDefinition.Folder = TEXT("Dialogues");

	// Set up dialogue tree (first node = root)
	OutDefinition.DialogueTree.RootNodeId = ParsedData.RootNodeID;

	// Add speakers
	for (const FString& SpeakerRef : ParsedData.Speakers)
	{
		FManifestDialogueSpeakerDefinition Speaker;
		Speaker.NPCDefinition = SpeakerRef;

		// Extract speaker ID from NPC_ name (e.g., "NPC_Blacksmith" -> "Blacksmith")
		FString SpeakerID = SpeakerRef;
		if (SpeakerID.StartsWith(TEXT("NPC_")))
		{
			SpeakerID = SpeakerID.RightChop(7);
		}
		Speaker.SpeakerID = SpeakerID;

		OutDefinition.Speakers.Add(Speaker);
	}

	// Convert nodes
	for (const FDialogueCSVRow& Row : ParsedData.Nodes)
	{
		FManifestDialogueNodeDefinition NodeDef = ConvertRowToNode(Row, ParsedData);
		OutDefinition.DialogueTree.Nodes.Add(NodeDef);
	}

	return true;
}

FManifestDialogueNodeDefinition FDialogueCSVParser::ConvertRowToNode(const FDialogueCSVRow& Row, const FParsedDialogueData& DialogueData)
{
	FManifestDialogueNodeDefinition NodeDef;

	NodeDef.Id = Row.NodeID;
	NodeDef.Type = Row.IsNPCNode() ? TEXT("npc") : TEXT("player");
	NodeDef.Text = Row.Text;

	// Speaker for NPC nodes
	if (Row.IsNPCNode() && !Row.Speaker.IsEmpty())
	{
		// Extract speaker ID from NPC_ name
		FString SpeakerID = Row.Speaker;
		if (SpeakerID.StartsWith(TEXT("NPC_")))
		{
			SpeakerID = SpeakerID.RightChop(7);
		}
		NodeDef.Speaker = SpeakerID;
	}

	// Option text for player nodes
	if (Row.IsPlayerNode())
	{
		NodeDef.OptionText = Row.OptionText.IsEmpty() ? Row.Text : Row.OptionText;
	}

	// Resolve replies
	ResolveReplies(Row, DialogueData, NodeDef.NPCReplies, NodeDef.PlayerReplies);

	// Parse events
	if (!Row.Events.IsEmpty())
	{
		NodeDef.Events = ParseEventsString(Row.Events);
	}

	// Parse conditions
	if (!Row.Conditions.IsEmpty())
	{
		NodeDef.Conditions = ParseConditionsString(Row.Conditions);
	}

	return NodeDef;
}

void FDialogueCSVParser::ResolveReplies(const FDialogueCSVRow& Row, const FParsedDialogueData& DialogueData,
                                        TArray<FString>& OutNPCReplies, TArray<FString>& OutPlayerReplies)
{
	if (Row.IsTerminal())
	{
		return;
	}

	TArray<FString> ReplyIDs;
	Row.Replies.ParseIntoArray(ReplyIDs, TEXT(";"), true);

	for (const FString& ReplyID : ReplyIDs)
	{
		FString CleanID = ReplyID.TrimStartAndEnd();

		if (CleanID.Equals(TEXT("END"), ESearchCase::IgnoreCase) ||
		    CleanID.Equals(TEXT("BACK"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		// Find the target node to determine reply type
		const FDialogueCSVRow* TargetNode = nullptr;
		for (const FDialogueCSVRow& Node : DialogueData.Nodes)
		{
			if (Node.NodeID.Equals(CleanID))
			{
				TargetNode = &Node;
				break;
			}
		}

		if (TargetNode)
		{
			if (TargetNode->IsNPCNode())
			{
				OutNPCReplies.Add(CleanID);
			}
			else
			{
				OutPlayerReplies.Add(CleanID);
			}
		}
	}
}

TArray<FManifestDialogueEventDefinition> FDialogueCSVParser::ParseEventsString(const FString& EventsStr)
{
	TArray<FManifestDialogueEventDefinition> Result;

	TArray<FString> EventParts;
	EventsStr.ParseIntoArray(EventParts, TEXT(";"), true);

	for (const FString& EventPart : EventParts)
	{
		FString CleanEvent = EventPart.TrimStartAndEnd();
		if (CleanEvent.IsEmpty())
		{
			continue;
		}

		FManifestDialogueEventDefinition EventDef;

		// Parse format: "NE_BeginQuest:Quest_Test" or just "NE_AddSaveCheckpoint"
		int32 ColonIndex;
		if (CleanEvent.FindChar(TEXT(':'), ColonIndex))
		{
			EventDef.Type = CleanEvent.Left(ColonIndex);
			FString Value = CleanEvent.RightChop(ColonIndex + 1);

			// Add to properties map
			EventDef.Properties.Add(TEXT("value"), Value);
		}
		else
		{
			EventDef.Type = CleanEvent;
		}

		EventDef.Runtime = TEXT("Start");
		Result.Add(EventDef);
	}

	return Result;
}

TArray<FManifestDialogueConditionDefinition> FDialogueCSVParser::ParseConditionsString(const FString& ConditionsStr)
{
	TArray<FManifestDialogueConditionDefinition> Result;

	TArray<FString> ConditionParts;
	ConditionsStr.ParseIntoArray(ConditionParts, TEXT(";"), true);

	for (const FString& ConditionPart : ConditionParts)
	{
		FString CleanCondition = ConditionPart.TrimStartAndEnd();
		if (CleanCondition.IsEmpty())
		{
			continue;
		}

		FManifestDialogueConditionDefinition ConditionDef;

		// Check for NOT prefix
		if (CleanCondition.StartsWith(TEXT("!")))
		{
			ConditionDef.bNot = true;
			CleanCondition = CleanCondition.RightChop(1);
		}

		// Parse format: "NC_IsQuestSucceeded:Quest_Test" or just "NC_IsDayTime"
		int32 ColonIndex;
		if (CleanCondition.FindChar(TEXT(':'), ColonIndex))
		{
			ConditionDef.Type = CleanCondition.Left(ColonIndex);
			FString Value = CleanCondition.RightChop(ColonIndex + 1);

			// Add to properties map
			ConditionDef.Properties.Add(TEXT("value"), Value);
		}
		else
		{
			ConditionDef.Type = CleanCondition;
		}

		Result.Add(ConditionDef);
	}

	return Result;
}
