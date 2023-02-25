//-------------------------------------------------------------------------------------
// File: TimeLimitAPI.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------
#pragma once
#ifndef TIME_LIMIT_API_H
#define TIME_LIMIT_API_H

#include <windows.h>

//-------------------------------------------------------------------------------------
// Name: TimeRemaining
// Desc: A structure used by the TimeLimitGetTimeRemaining() function to report the time
//       remaining before an imminent lock out.
//-------------------------------------------------------------------------------------
struct TimeRemaining
{
    unsigned char days;
    unsigned char hours;
    unsigned char minutes;
    unsigned char seconds;
};

//-------------------------------------------------------------------------------------
// Name: HourlyRestrictions
// Desc: A structure used by the TimeLimitGetHourlyRestrictions() function to report
//       the hourly restrictions for the current user.  The hourly restrictions are
//       expressed as a bit field using bits 0-23 (bit 0 is 12am, bit 23 is 11pm).  An
//       unset bit represents a restricted hour.
//-------------------------------------------------------------------------------------
struct HourlyRestrictions
{
    union
    {
        unsigned int days[7];
        struct
        {
            unsigned int sun;
            unsigned int mon;
            unsigned int tue;
            unsigned int wed;
            unsigned int thu;
            unsigned int fri;
            unsigned int sat;
        } dayOfWeek;
    };
};


//-------------------------------------------------------------------------------------
// API Function Prototypes
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
// Name:    TimeLimitEnabled()
// Desc:    Checks if the current user has time limit restrictions.
// Returns: TRUE if the user has time restrictions, FALSE if not.
// Note:    The return value will always be FALSE if the operating system is less than
//          Windows Vista as Parental Controls do not exist on previous versions of
//          Windows.
//-------------------------------------------------------------------------------------
BOOL TimeLimitEnabled();

//-------------------------------------------------------------------------------------
// Name:    TimeLimitGetTimeRemaining()
// Desc:    Gets the time remaining until the current user is locked out of their session
//          due to a restricted hour.
// Returns: An HRESULT.
//              S_OK - A lock out is imminent or is currently in effect.  pTime will
//                     be filled with the time remaining.  If all fields in the pTime
//                     structure are 0, then it is assumed that the user is currently
//                     locked out.
//              S_FALSE - No restricted hours were found.  Values in the pTime structure
//                     remain untouched.
//              E_INVALIDARG - pTime is NULL.
//              E_FAIL - The function could not determine the current time.
//------------------------------------------------------------------------------------
HRESULT TimeLimitGetTimeRemaining( TimeRemaining* pTime );

//------------------------------------------------------------------------------------
// Name:    TimeLimitGetSecondsRemaining()
// Desc:    Gets the time, in seconds, until the current user is locked out due to time
//          restrictions.
// Returns: If the return value is < 0, then there aren't any time restrictions or
//          there was an error.  If the value is 0, then it is assumed that the user
//          is currently locked out.  Any value > 0 is the time, in seconds, until
//          the next restricted hour.
//------------------------------------------------------------------------------------
INT TimeLimitGetSecondsRemaining();

//------------------------------------------------------------------------------------
// Name:    TimeLimitGetHourlyRestrictions()
// Desc:    Gets the restricted hours for the current user.
// Returns: An HRESULT.
//              S_OK - The current user has time restrictions.  The pRestrictions
//                     structure will contain masks that reflect the restrictions.
//              E_FAIL - The current user does not have time restrictions or there
//                     was an error.  pRestrictions will contain 0x00ffffff as mask
//                     values (which means that each hour is unrestricted).
//------------------------------------------------------------------------------------
HRESULT TimeLimitGetHourlyRestrictions( HourlyRestrictions* pRestrictions );


//-------------------------------------------------------------------------------------
// Inline Implementation
//-------------------------------------------------------------------------------------
inline BOOL TimeLimitEnabled()
{
    HourlyRestrictions restrictions;
    return SUCCEEDED( TimeLimitGetHourlyRestrictions( &restrictions ) );
}

#endif  // TIME_LIMIT_API_H
