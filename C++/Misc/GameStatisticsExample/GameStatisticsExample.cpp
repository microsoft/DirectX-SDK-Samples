//--------------------------------------------------------------------------------------
// GameStatisticsExample.cpp
//
// Sample code shows off the Windows 7 Game Explorer APIs - IGameStatistics and IGameStatisticsMgr.
// The statistics can be seen in Windows 7 version of Game Explorer
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#define _WIN32_DCOM

#include <windows.h>
#include <rpcsal.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <atlbase.h>
#include <atlcom.h>
#include <strsafe.h>
#include <assert.h>
#include <gameux.h>

//--------------------------------------------------------------------------------------
// Name: GetGDFBinaryPath
// Desc: Gets the path of the GDF binary.  This sample uses Minesweeper for an example 
//       which is installed on most machines.  Change this to point to the GDF path of your title.
//--------------------------------------------------------------------------------------
HRESULT GetGDFBinaryPath(WCHAR* szGDFPath, int cchLength)
{
    assert(szGDFPath);
    HRESULT hr;
    
    hr = SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, szGDFPath);    
    if (SUCCEEDED(hr))
    {
        hr = StringCchCat(szGDFPath, cchLength, L"\\Microsoft Games\\Minesweeper\\Minesweeper.exe"); 
    }
    
    return hr;
}

//--------------------------------------------------------------------------------------
// Name: WriteStatistics
// Desc: Writes statistics 
//--------------------------------------------------------------------------------------
HRESULT WriteStatistics(IGameStatistics* piStats)
{
    assert(piStats);
    if (piStats == NULL)
    {
        return E_INVALIDARG;
    }

    HRESULT hr;
    
    // These are example categories.  You can set up to 10 categories and name them as appropriate.
    hr = piStats->SetCategoryTitle(0, L"Beginner");             
    if (SUCCEEDED(hr))
    {
        hr = piStats->SetCategoryTitle(1, L"Intermediate");                 
        if (SUCCEEDED(hr))
        {
            hr = piStats->SetCategoryTitle(2, L"Advanced");                
        }
    }
    
    if (FAILED(hr))
        return hr;

    // Statistic data would typically come from the game state and would be dependent on the category 
    // As a trivial example, we will write the same fixed data to all 3 categories
    for(WORD iCategory = 0; iCategory < 3; iCategory++)
    {
        hr = piStats->SetStatistic(iCategory, 0, L"Best Time", L"150 second");                 
        if (SUCCEEDED(hr))
        {
            hr = piStats->SetStatistic(iCategory, 1, L"Games Played", L"40");
            if (SUCCEEDED(hr))
            {
                hr = piStats->SetStatistic(iCategory, 2, L"Games Won", L"20");
                if (SUCCEEDED(hr))
                {
                    hr = piStats->SetStatistic(iCategory, 3, L"Win Percentage", L"50%");
                    if (SUCCEEDED(hr))
                    {
                        hr = piStats->SetStatistic(iCategory, 4, L"Longest Winning Streak", L"3");
                        if (SUCCEEDED(hr))
                        {
                            hr = piStats->SetStatistic(iCategory, 5, L"Longest Losing Streak", L"4");
                            if (SUCCEEDED(hr))
                            {
                                hr = piStats->SetStatistic(iCategory, 6, L"Current Streak", L"2");
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (SUCCEEDED(hr))
    {
        hr = piStats->SetLastPlayedCategory(2);             
        if (SUCCEEDED(hr))
        {
            hr = piStats->Save(TRUE);                       
        }
    }
    
    return hr;
}

//--------------------------------------------------------------------------------------
// Name: DisplayStatistics
// Desc: Display the statistics.  As an trivial example, this function builds a large string
//       and display the data in a message box
//--------------------------------------------------------------------------------------
HRESULT DisplayStatistics(IGameStatistics* piStats)
{
    assert(piStats);
    if (piStats == NULL)
    {
        return E_INVALIDARG;
    }

    WORD wMaxCategories = 0;
    WCHAR szStats[1024] = {0};
    HRESULT hr = piStats->GetMaxCategories(&wMaxCategories);                        
    for (WORD wIndex = 0; wIndex < wMaxCategories && SUCCEEDED(hr); wIndex++)
    {
        LPWSTR pCategory = NULL;
        hr = piStats->GetCategoryTitle(wIndex, &pCategory);                         
        if (SUCCEEDED(hr) && pCategory)
        {
            // If the category title is not NULL, display the statistics for this category
            StringCchCat(szStats, ARRAYSIZE(szStats)-1, pCategory);
            StringCchCat(szStats, ARRAYSIZE(szStats)-1, L"\n");

            WORD wMaxStats;
            LPOLESTR pName = NULL;
            LPOLESTR pValue = NULL;    
            hr = piStats->GetMaxStatsPerCategory(&wMaxStats);                       
            for (WORD wStartIndex = 0; wStartIndex < wMaxStats && SUCCEEDED(hr); wStartIndex++)
            {
                hr = piStats->GetStatistic(wIndex, wStartIndex, &pName, &pValue);   
                if (SUCCEEDED(hr) && pName && pValue)
                {
                    StringCchCat(szStats, ARRAYSIZE(szStats)-1, L"\t");
                    StringCchCat(szStats, ARRAYSIZE(szStats)-1, pName);             
                    StringCchCat(szStats, ARRAYSIZE(szStats)-1, L": ");
                    StringCchCat(szStats, ARRAYSIZE(szStats)-1, pValue);            
                    StringCchCat(szStats, ARRAYSIZE(szStats)-1, L"\n");
                    CoTaskMemFree(pName);
                    CoTaskMemFree(pValue);
                }
            }
            CoTaskMemFree(pCategory);
        }
    }
    
    MessageBox(NULL, szStats, L"Game Statistics", MB_OK);       
    
    return hr;
}

//--------------------------------------------------------------------------------------
// Name: wmain
// Desc: Main function
//--------------------------------------------------------------------------------------
int __cdecl wmain(int argc, __in_ecount(argc) wchar_t *argv[ ], __in wchar_t *)
{
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        WCHAR szGDFPath[MAX_PATH];
        hr = GetGDFBinaryPath(szGDFPath, MAX_PATH);                                        
        if (SUCCEEDED(hr) )       
        {
            BSTR bstrGDFPath = SysAllocString(szGDFPath);
            if( bstrGDFPath )
            {           
                CComPtr<IGameStatisticsMgr> spiMgr;
                hr = spiMgr.CoCreateInstance(__uuidof(GameStatistics));                 
                if (SUCCEEDED(hr))
                {
                    GAMESTATS_OPEN_TYPE ot = GAMESTATS_OPEN_OPENORCREATE;
                    GAMESTATS_OPEN_RESULT res;
                    CComPtr<IGameStatistics> spiStats;                    
                    hr = spiMgr->GetGameStatistics(bstrGDFPath, ot, &res, &spiStats);   
                    if(SUCCEEDED(hr))
                    {
                        if (res == GAMESTATS_OPEN_CREATED)
                        {
                            // It is a newly created statistics file
                            // In this sample we will write some example data, and display them
                            // They can then be seen in the Windows 7 Game Explorer
                            hr = WriteStatistics(spiStats);
                            if(SUCCEEDED(hr))
                            {
                                hr = DisplayStatistics(spiStats);                       
                            }
                            
                            // Uncomment this line  to remove the statistics
                            //hr = spiMgr->RemoveGameStatistics(bstrGDFPath);     
                        }
                        else
                        {
                            // If the statistics file already exists, then in this 
                            // sample we will just display them
                            hr = DisplayStatistics(spiStats);                           
                        }
                    }
                }
                
                SysFreeString(bstrGDFPath);
            }
        }
        CoUninitialize();
    }

    if (SUCCEEDED(hr))
    {
        wprintf(L"SUCCESS\n");
    }
    else
    {
        wprintf(L"FAILURE\n");
    }
    return 0;
}
