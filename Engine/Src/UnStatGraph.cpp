/*=============================================================================
	UnStatGraph.cpp: Stats Graphing Utility
	Copyright 2002 Epic MegaGames, Inc. This software is a trade secret.
	Revision history:
		* Created by James Golding
=============================================================================*/

#include "EnginePrivate.h"

ENGINE_API FStatGraph* GStatGraph;


// Size of history for each line
#define STATGRAPH_HISTORY_SIZE 256


FStatGraph::FStatGraph()
{
	bHideGraph = 1;
	bLockScale = 0;

	GraphSize[0] = 250;
	GraphSize[1] = 150;
	
	GraphOrigin[0] = 100;
	GraphOrigin[1] = 200;
	
	XRange=100;
	bHideKey=0;
	ZeroYRatio=0.33f;
	BackgroundAlpha=100;

	// added by Demiurge Studios (StatGraph)
	bShowBarGraphs = 0;
	statBar1 = NULL;
	statBar2 = NULL;
	// end Demiurge
}

FStatGraph::~FStatGraph()
{
	// added by Demiurge Studios (StatGraph)
	if(statBar1) delete statBar1;
	if(statBar2) delete statBar2;
	// end Demiurge
	
}

void FStatGraph::AddLine(FString LineName, FColor Color, FLOAT YRangeMin, FLOAT YRangeMax)
{
	guard(FStatGraph::AddLine);

	FStatGraphLine* line = new(Lines) FStatGraphLine();

	// Should probably be in constructor...
	line->bHideLine = 0;
	line->DataHistory.AddZeroed(STATGRAPH_HISTORY_SIZE);
	line->DataPos = 0;
	line->LineColor = Color;
	line->LineName = LineName;
	line->YRange[0] = YRangeMin;
	line->YRange[1] = YRangeMax;
	line->XSpacing = 0.2f;
	line->bAutoScale = 0;

	// Add to name->line map
	LineNameMap.Set(*(line->LineName), Lines.Num()-1);

	unguard;
}

void FStatGraph::AddLineAutoRange(FString LineName, FColor Color)
{
	guard(FStatGraph::AddLineAutoRange);

	// Create a line in the usual way, but with zero range. Then set AutoRange to true.
	AddLine(LineName, Color, 0, 0);
	INT* lineIx = LineNameMap.Find(LineName);
	check(lineIx && *lineIx < Lines.Num());
	FStatGraphLine* line = &Lines(*lineIx);
	check(line);

	line->bAutoScale = 1;

	unguard;
}

// Remove all graph lines.
void FStatGraph::Reset()
{
	guard(FStatGraph::Reset);
	
	Lines.Empty();
	LineNameMap.Empty();

	unguard;
}

void FStatGraph::AddDataPoint(FString LineName, FLOAT Data, UBOOL bAutoAddLine)
{
	guard(FStatGraph::AddDataPoint);

	// Try and find the existing line for this name.
	INT* lineIx = LineNameMap.Find(LineName);
	FStatGraphLine* line;

	// If we didn't find it, add one automatically if desired.
	if(!lineIx && bAutoAddLine)
	{
		// Pick a hue and do HSV->RGB
		//BYTE Hue = appRound(FRange(0, 255).GetRand());
		BYTE Hue = (Lines.Num() * 40)%255;
		FColor randomColor(FGetHSV(Hue, 128, 255));
		randomColor.A = 255;
		
		AddLineAutoRange(LineName, randomColor);

		lineIx = LineNameMap.Find(LineName);
		check(lineIx && *lineIx < Lines.Num());
	}

	if(lineIx)
	{
		line = &Lines(*lineIx);
		check(line);
	}
	else
		return;

	line->DataHistory(line->DataPos) = Data;
	line->DataPos++;
	if(line->DataPos > STATGRAPH_HISTORY_SIZE-1)
		line->DataPos = 0;

	if(line->bAutoScale)
	{
		line->YRange[0] = Min(line->YRange[0], Data);
		line->YRange[1] = Max(line->YRange[1], Data);
	}

	unguard;
}

// Given the name of a stat, find its index
// Currently only looks in DWORD stats - seems to be what all stats are.
static INT FindStat(FString StatName)
{
	INT statIx = -1;

	for(INT i=0; i<GStats.DWORDStatsSectionDescriptions.Num() && statIx == -1; i++)
	{
		if(GStats.DWORDStatsSectionDescriptions(i) == StatName)
		{
			statIx = i;
		}
	}

	return statIx;	
}

static void ReScale(FStatGraph* graph)
{
	for(INT i=0; i<graph->Lines.Num(); i++)
	{
		FStatGraphLine* line = &graph->Lines(i);

		if(!line->bAutoScale)
			continue;

		line->YRange[0] = 0;
		line->YRange[1] = 0;

		for(INT j=0; j<STATGRAPH_HISTORY_SIZE; j++)
		{
			line->YRange[0] = Min(line->YRange[0], line->DataHistory(j));
			line->YRange[1] = Max(line->YRange[1], line->DataHistory(j));
		}
	}	
}

// added by Demiurge Studios (StatGraph)
void FStatGraph::RemoveStat(FString StatName)
{
	INT* lineIx = LineNameMap.Find(StatName);
	FStatGraphLine* line = &Lines(*lineIx);
	line->bHideLine = true;

	/*
	INT StatIndex = FindStat(StatName);
	if(StatIndex != -1)
	{
		INT index = GetStat.FindItemIndex(StatIndex);
		GetStat.Remove(index);

		INT* lineIx = LineNameMap.Find(StatName);
		FStatGraphLine* line = &Lines(*lineIx);
		INT lineIx2 = Lines.FindItemIndex(*line);
		Lines.Remove(lineIx2);
		LineNameMap.Remove(*(StatName));
		// TODO Mem leak?
	}
	*/
}
// end Demiurge

UBOOL FStatGraph::Exec(const TCHAR* Cmd, FOutputDevice& Ar)
{
	guard(FStatGraph::Exec);
	
	if( ParseCommand(&Cmd,TEXT("GRAPH")) )
	{
		if( ParseCommand(&Cmd,TEXT("SHOW")) )
		{
			bHideGraph = !bHideGraph;
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("KEY")) )
		{
			bHideKey = !bHideKey;
			return 1;
		}
		else if( ParseCommand(&Cmd, TEXT("LOCKSCALE")) )
		{
			bLockScale = !bLockScale;
			return 1;
		}
		else if( ParseCommand(&Cmd, TEXT("RESCALE")) )
		{
			ReScale(this);
			return 1;
		}

		// added by Demiurge Studios (StatGraph)

		//this allows one to know the hidden state of the window
		else if( ParseCommand(&Cmd, TEXT("HIDE")) )
		{
			bHideGraph = true;
			return 1;
		}
		else if( ParseCommand(&Cmd, TEXT("UNHIDE")) )
		{
			bHideGraph = false;
			return 1;
		}

		FString sName;
		Parse(Cmd, TEXT("REMOVESTAT="), sName);
		INT statIndex = FindStat(sName);
		if(statIndex != -1)
		{
			if(GetStat.FindItemIndex(statIndex) != INDEX_NONE)
			{
				RemoveStat(sName);
			}
		}
		// end Demiurge

		Parse(Cmd, TEXT("XRANGE="), XRange);
		Parse(Cmd, TEXT("XSIZE="), GraphSize[0]);
		Parse(Cmd, TEXT("YSIZE="), GraphSize[1]);
		Parse(Cmd, TEXT("XPOS="), GraphOrigin[0]);
		Parse(Cmd, TEXT("YPOS="), GraphOrigin[1]);
		Parse(Cmd, TEXT("ALPHA="), BackgroundAlpha);
		Parse(Cmd, TEXT("FILTER="), FilterString);

		if( FilterString == FString(TEXT("None")) )
			FilterString = TEXT("");

		FString StatName;
		Parse(Cmd, TEXT("ADDSTAT="), StatName);
		INT statIx = FindStat(StatName);
		if(statIx != -1)
		// added by Demiurge Studios (StatGraph)
		{
			if(GetStat.FindItemIndex(statIx) != INDEX_NONE)
			{
				INT* lineIx = LineNameMap.Find(StatName);
				FStatGraphLine* line = &Lines(*lineIx);
				line->bHideLine = false;
			}
			else
			{
				GetStat.AddItem(statIx);
				AddDataPoint(GStats.DWORDStatsSectionDescriptions(statIx), GStats.DWORDOldStats(statIx), 1);
			}
		}
		// end Demiurge

		return 1;
	}

	// Added by Demiurge Studios (StatGraph)
	else if( ParseCommand(&Cmd,TEXT("BARGRAPH")) )
	{
		if( ParseCommand(&Cmd,TEXT("SHOW")) )
		{
			bShowBarGraphs = !bShowBarGraphs;
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("HIDE")) )
		{
			bShowBarGraphs = false;
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("UNHIDE")) )
		{
			bShowBarGraphs = true;
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("ADDSTAT")) )
		{
			FString StatName;
			Parse(Cmd, TEXT("="), StatName);
			INT statIx = FindStat(StatName);

			if(statIx != -1)
			{
				if(BarStats.FindItemIndex(statIx) != INDEX_NONE)
				{
					BarStats.Remove(BarStats.FindItemIndex(statIx));
					BarStatMap.Remove(*(StatName));
				}
				BarStats.AddItem(statIx);
				BarStat b;
				b.statIndex = statIx;
				Parse(Cmd, TEXT("MAX="), b.desiredMax);
				Parse(Cmd, TEXT("SMALL="), b.smallIsGood);
				
				BarStatMap.Set(*(StatName), b);
			}
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("REMOVESTAT")) )
		{
			FString StatName;
			Parse(Cmd, TEXT("="), StatName);
			INT statIx = FindStat(StatName);

			if(statIx != -1)
			{
				if(BarStats.FindItemIndex(statIx) != INDEX_NONE)
				{
					BarStats.Remove(BarStats.FindItemIndex(statIx));
					BarStatMap.Remove(*(StatName));
				}
				if(BarStats.Num() == 0)
					bShowBarGraphs = false;
			}
			return 1;
		}
		return 0;
	}
        // end Demiurge

	else
		return 0;
	unguard;
}

#define BG_BORDER			12
#define BG_KEY_GAP			8
#define BG_KEY_WID			100
#define BG_KEY_LINESPACE	10

void FStatGraph::Render(UViewport* viewport, FRenderInterface* RI)
{
	guard(FStatGraph::Render);
	
	// Changed by Demiurge (StatGraph)
	FCanvasUtil CanvasUtil(&viewport->RenderTarget, RI);
	UCanvas* Canvas = viewport->Canvas;
	Canvas->SetClip(0,0,viewport->SizeX,viewport->SizeY);

	if(!bHideGraph)
	{
		// Grab any global stats that we've been told to.
		for(INT i=0; i<GetStat.Num(); i++)
		{
			INT statIx = GetStat(i);
			AddDataPoint(GStats.DWORDStatsSectionDescriptions(statIx), GStats.DWORDOldStats(statIx) * 1000.0f * GSecondsPerCycle, 1);
		}
		// End Demiurge

		// graph background
		DECLARE_STATIC_UOBJECT( UConstantColor, GraphBackground, { } );
		DECLARE_STATIC_UOBJECT( UFinalBlend, GraphBackgroundBlend, { } );

		GraphBackground->Color = FColor(BackgroundAlpha, BackgroundAlpha, BackgroundAlpha);
		GraphBackgroundBlend->Material = GraphBackground;
		GraphBackgroundBlend->FrameBufferBlending = FB_Modulate;

		Canvas->DrawTile(GraphBackgroundBlend, 
			GraphOrigin[0]-BG_BORDER, 
			GraphOrigin[1]+BG_BORDER, 
			GraphSize[0]+(2*BG_BORDER), 
			-(GraphSize[1]+(2*BG_BORDER)), 
			0, 0, 0, 0, 1.f, FPlane(1.f,1.f,1.f,1.f), FPlane(0,0,0,0));

		if(!bHideKey)
		{
			Canvas->DrawTile(GraphBackgroundBlend, 
				GraphOrigin[0]+GraphSize[0]+BG_BORDER+BG_KEY_GAP, 
				GraphOrigin[1]+BG_BORDER, 
				BG_KEY_WID + (2*BG_BORDER), 
				-(GraphSize[1]+(2*BG_BORDER)), 
				0, 0, 0, 0, 1.f, FPlane(1.f,1.f,1.f,1.f), FPlane(0,0,0,0));
		}

		FLOAT ZeroY = GraphOrigin[1] - (GraphSize[1] * ZeroYRatio);

		// Graph y axis
		CanvasUtil.DrawLine(GraphOrigin[0], GraphOrigin[1], 
			GraphOrigin[0], GraphOrigin[1]-GraphSize[1], 
			FColor(255,255,255));
		
		// Draw x axis.
		CanvasUtil.DrawLine(GraphOrigin[0], ZeroY, 
			GraphOrigin[0]+GraphSize[0], ZeroY, 
			FColor(255,255,255));	

		UBOOL doFilter = (FilterString.Len() > 0);

		// Find overall max and min y (used if bLockScale == true)
		FLOAT TotalYRange[2] = {0, 0};
		if(bLockScale)
		{
			for(INT i=0; i<Lines.Num(); i++)
			{
				FStatGraphLine* line = &Lines(i);

				if(line->bHideLine || (doFilter && line->LineName.Caps().InStr(FilterString.Caps()) == -1))
					continue;

				TotalYRange[0] = Min(TotalYRange[0], line->YRange[0]);
				TotalYRange[1] = Max(TotalYRange[1], line->YRange[1]);
			}
		}

		// Draw data line.
		INT drawCount = 0;
		for(INT i=0; i<Lines.Num(); i++)
		{
			FStatGraphLine* line = &Lines(i);

			// If this line is hidden, or its name doesn't match the filter, skip.
			if(line->bHideLine || (doFilter && line->LineName.Caps().InStr(FilterString.Caps()) == -1))
				continue;
		
			// Draw key entry if desired.
			if(!bHideKey)
			{
				TCHAR keyEntry[1024];
				appSprintf(keyEntry, TEXT("%s: %f"), *(line->LineName), line->YRange[1]);

				Canvas->CurX = GraphOrigin[0] + GraphSize[0] + (2*BG_BORDER) + BG_KEY_GAP;
				Canvas->CurY = GraphOrigin[1] - GraphSize[1] + (BG_KEY_LINESPACE*(drawCount));
				Canvas->Color = line->LineColor;
				Canvas->WrappedPrintf(Canvas->SmallFont, 0, keyEntry);
			}

			// Factor to scale all data by to fit onto same graph as other lines.
			FLOAT lineScale, s1=1000000000.0f, s2=1000000000.0f;
		
			if(bLockScale)
			{
				// Max scale to make minimum fit on graph
				if(line->YRange[0] < -0.001f)
					s1 = -(ZeroYRatio*GraphSize[1])/TotalYRange[0];
				
				// Max scale to make maximum fit on graph
				if(line->YRange[1] > 0.001f)
					s2 = ((1-ZeroYRatio)*GraphSize[1])/TotalYRange[1];			
			}
			else
			{
				// Max scale to make minimum fit on graph
				if(line->YRange[0] < -0.001f)
					s1 = -(ZeroYRatio*GraphSize[1])/line->YRange[0];
				
				// Max scale to make maximum fit on graph
				if(line->YRange[1] > 0.001f)
					s2 = ((1-ZeroYRatio)*GraphSize[1])/line->YRange[1];
			}
			

			lineScale = Min(s1, s2);

			// Start drawing from most recent data (right hand edge)
			INT dataPos = line->DataPos-1;
			if(dataPos == -1)
				dataPos = STATGRAPH_HISTORY_SIZE-1;

			INT oldDataPos = dataPos;

			FLOAT xPos = GraphOrigin[0] + GraphSize[0];
			FLOAT oldXPos = xPos;

			// Keep working from right to left until we hit y axis, or run out of history.
			while(xPos > GraphOrigin[0] && dataPos != (line->DataPos) )
			{
				FLOAT y0 = line->DataHistory(dataPos) * lineScale;
				FLOAT y1 = line->DataHistory(oldDataPos) * lineScale;

				CanvasUtil.DrawLine(xPos, 
					ZeroY - y0, 
					oldXPos, 
					ZeroY - y1, 
					line->LineColor);

				// Move back in time/left along graph
				oldXPos = xPos;
				xPos -= (GraphSize[0]/XRange);

				oldDataPos = dataPos;
				dataPos--;
				if(dataPos == -1)
					dataPos = STATGRAPH_HISTORY_SIZE-1;
			}

			drawCount++;
		}
    // Added by Demiurge (StatGraph)
	}
	
	if(bShowBarGraphs)
	{
		if(!statBar1 || !statBar2)
		{
			statBar1 = Cast<UMaterial>(UObject::StaticLoadObject( UMaterial::StaticClass(), NULL, *FString(TEXT("Engine.Bar1")), NULL, LOAD_NoWarn | RF_Native, NULL ));
			statBar1->SetFlags(statBar1->GetFlags() | RF_Native);
			statBar2 = Cast<UMaterial>(UObject::StaticLoadObject( UMaterial::StaticClass(), NULL, *FString(TEXT("Engine.Bar2")), NULL, LOAD_NoWarn | RF_Native, NULL ));
			statBar2->SetFlags(statBar2->GetFlags() | RF_Native);
		}
		INT x = Canvas->Viewport->SizeX - 270;
		INT y = Canvas->Viewport->SizeY - 40;
		Canvas->Color = FColor(255,255,255);

		FString Dummy;

		for(INT i = 0; i < BarStats.Num(); i++)
		{
			INT statIndex = BarStats(i);
			BarStat* b;
			b = BarStatMap.Find(GStats.DWORDStatsSectionDescriptions(statIndex));

			TCHAR statName[40];
			appStrcpy(statName, *(GStats.DWORDStatsSectionDescriptions(statIndex)));
			
			FLOAT currentValue;
			if(GStats.AllStats(statIndex).StatsUnit == STATSUNIT_MSec)  //TODO - see if we can avoid using AllStats
			{
				currentValue = GStats.DWORDOldStats(statIndex) * GSecondsPerCycle * 1000;
				appStrcat(statName, TEXT(" ms"));
			}
			else
			{
				currentValue = GStats.DWORDOldStats(statIndex);
			}

			INT width;
			if(b->desiredMax != 0)
				width = currentValue / b->desiredMax *  150;
			else
				width = 0.0;

		
			if(width < 1)
				width = 1;
			else if(width > 255)
				width = 255;

			Canvas->CurX = x - 10;
			Canvas->CurY = y+10;
			Canvas->Font = Canvas->SmallFont;
			Canvas->DrawTextJustified(2, 0, Canvas->CurY, Canvas->CurX, Canvas->CurY+18, statName);
			Canvas->CurX = x;
			Canvas->CurY = y;
			Dummy = FString::Printf(TEXT("%f"), currentValue);
			Canvas->WrappedPrintf(Canvas->SmallFont,0, *Dummy);

			Canvas->CurX = x + 150;
			Canvas->CurY = y;
			Dummy = FString::Printf(TEXT("%f"),b->desiredMax);
			Canvas->WrappedPrintf(Canvas->SmallFont,0, *Dummy);

			if(b->smallIsGood)
				Canvas->DrawTile(
					statBar1,
					x,
					y+10,
					width,
					12,
					0,0,
					width,
					12,
					Canvas->Z,
					Canvas->Color.Plane(),
					FPlane(0,0,0,0)
				);
			else
				Canvas->DrawTile(
					statBar2,
					x,
					y+10,
					width,
					12,
					0,0,
					width,
					12,
					Canvas->Z,
					Canvas->Color.Plane(),
					FPlane(0,0,0,0)
				);

			y -= 30;
		}
	
		Canvas->CurX = x;
		Canvas->CurY = y;
		Dummy = FString::Printf(TEXT("Stat Bar Graphs"));
		Canvas->WrappedPrintf(Canvas->SmallFont,0, *Dummy);

	}
	// end Demiurge

	unguard;
}

// // // //

FStatGraphLine::FStatGraphLine()
{
	
}

