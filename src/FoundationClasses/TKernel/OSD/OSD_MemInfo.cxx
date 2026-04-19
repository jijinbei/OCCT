// Created on: 2011-10-05
// Created by: Kirill GAVRILOV
// Copyright (c) 2013-2014 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#if (defined(_WIN32) || defined(__WIN32__))
  #include <windows.h>
  #include <winbase.h>
  #include <process.h>
  #include <malloc.h>
  #include <psapi.h>
  #ifdef _MSC_VER
    #pragma comment(lib, "Psapi.lib")
  #endif
#elif (defined(__APPLE__))
  #include <mach/task.h>
  #include <mach/mach.h>
  #include <malloc/malloc.h>
#else
  #include <unistd.h>
  #include <malloc.h>
#endif

#include <OSD_MemInfo.hxx>

#if defined(__EMSCRIPTEN__)
  #include <emscripten.h>

//! Return WebAssembly heap size in bytes.
EM_JS(double, OSD_MemInfo_getModuleHeapLength, (), { return Module.HEAP8.length; });
#endif

//=================================================================================================

OSD_MemInfo::OSD_MemInfo(const bool theImmediateUpdate)
{
  SetActive(true);
  if (theImmediateUpdate)
  {
    Update();
  }
  else
  {
    Clear();
  }
}

//=================================================================================================

void OSD_MemInfo::SetActive(const bool theActive)
{
  for (size_t anIter = 0; anIter < static_cast<size_t>(Counter::MemCounter_NB); ++anIter)
  {
    SetActive(static_cast<Counter>(anIter), theActive);
  }
}

//=================================================================================================

void OSD_MemInfo::Clear()
{
  for (size_t anIter = 0; anIter < static_cast<size_t>(Counter::MemCounter_NB); ++anIter)
  {
    myCounters[anIter] = size_t(-1);
  }
}

//=================================================================================================

void OSD_MemInfo::Update()
{
  Clear();
#ifndef OCCT_UWP
  #if defined(_WIN32)
    #if (_WIN32_WINNT >= 0x0500)
  if (IsActive(Counter::MemVirtual))
  {
    MEMORYSTATUSEX aStatEx;
    aStatEx.dwLength = sizeof(aStatEx);
    GlobalMemoryStatusEx(&aStatEx);
    myCounters[size_t(Counter::MemVirtual)] =
      size_t(aStatEx.ullTotalVirtual - aStatEx.ullAvailVirtual);
  }
    #else
  if (IsActive(Counter::MemVirtual))
  {
    MEMORYSTATUS aStat;
    aStat.dwLength = sizeof(aStat);
    GlobalMemoryStatus(&aStat);
    myCounters[size_t(Counter::MemVirtual)] = size_t(aStat.dwTotalVirtual - aStat.dwAvailVirtual);
  }
    #endif

  if (IsActive(Counter::MemPrivate) || IsActive(Counter::MemWorkingSet)
      || IsActive(Counter::MemWorkingSetPeak) || IsActive(Counter::MemSwapUsage)
      || IsActive(Counter::MemSwapUsagePeak))
  {
    // use Psapi library
    HANDLE aProcess = GetCurrentProcess();
    #if (_WIN32_WINNT >= 0x0501)
    PROCESS_MEMORY_COUNTERS_EX aProcMemCnts;
    #else
    PROCESS_MEMORY_COUNTERS aProcMemCnts;
    #endif
    if (GetProcessMemoryInfo(aProcess,
                             (PROCESS_MEMORY_COUNTERS*)&aProcMemCnts,
                             sizeof(aProcMemCnts)))
    {
    #if (_WIN32_WINNT >= 0x0501)
      myCounters[size_t(Counter::MemPrivate)] = aProcMemCnts.PrivateUsage;
    #endif
      myCounters[size_t(Counter::MemWorkingSet)]     = aProcMemCnts.WorkingSetSize;
      myCounters[size_t(Counter::MemWorkingSetPeak)] = aProcMemCnts.PeakWorkingSetSize;
      myCounters[size_t(Counter::MemSwapUsage)]      = aProcMemCnts.PagefileUsage;
      myCounters[size_t(Counter::MemSwapUsagePeak)]  = aProcMemCnts.PeakPagefileUsage;
    }
  }

  if (IsActive(Counter::MemHeapUsage))
  {
    _HEAPINFO hinfo;
    int       heapstatus;
    hinfo._pentry = nullptr;

    myCounters[size_t(Counter::MemHeapUsage)] = 0;
    while ((heapstatus = _heapwalk(&hinfo)) == _HEAPOK)
    {
      if (hinfo._useflag == _USEDENTRY)
      {
        myCounters[size_t(Counter::MemHeapUsage)] += hinfo._size;
      }
    }
  }

  #elif defined(__EMSCRIPTEN__)
  if (IsActive(Counter::MemHeapUsage) || IsActive(Counter::MemWorkingSet)
      || IsActive(Counter::MemWorkingSetPeak))
  {
    // /proc/%d/status is not emulated - get more info from mallinfo()
    const struct mallinfo aMI = mallinfo();
    if (IsActive(Counter::MemHeapUsage))
    {
      myCounters[size_t(Counter::MemHeapUsage)] = aMI.uordblks;
    }
    if (IsActive(Counter::MemWorkingSet))
    {
      myCounters[size_t(Counter::MemWorkingSet)] = aMI.uordblks;
    }
    if (IsActive(Counter::MemWorkingSetPeak))
    {
      myCounters[size_t(Counter::MemWorkingSetPeak)] = aMI.usmblks;
    }
  }
  if (IsActive(Counter::MemVirtual))
  {
    myCounters[size_t(Counter::MemVirtual)] = (size_t)OSD_MemInfo_getModuleHeapLength();
  }
  #elif (defined(__linux__) || defined(__linux))
  if (IsActive(Counter::MemHeapUsage))
  {
    #if defined(__GLIBC__)
      #define HAS_MALLINFO
      #if defined(__GLIBC_PREREQ) && __GLIBC_PREREQ(2, 33)
        #define HAS_MALLINFO2
      #endif
    #endif

    #ifdef HAS_MALLINFO
      #ifdef HAS_MALLINFO2
    const struct mallinfo2 aMI = mallinfo2();
      #else
    const struct mallinfo aMI = mallinfo();
      #endif
    myCounters[size_t(Counter::MemHeapUsage)] = aMI.uordblks;
    #else
    myCounters[size_t(Counter::MemHeapUsage)] = 0;
    #endif
  }

  if (!IsActive(Counter::MemVirtual) && !IsActive(Counter::MemWorkingSet)
      && !IsActive(Counter::MemWorkingSetPeak) && !IsActive(Counter::MemPrivate))
  {
    return;
  }

  // use procfs on Linux
  char aBuff[4096];
  snprintf(aBuff, sizeof(aBuff), "/proc/%d/status", getpid());
  std::ifstream aFile;
  aFile.open(aBuff);
  if (!aFile.is_open())
  {
    return;
  }

  while (!aFile.eof())
  {
    memset(aBuff, 0, sizeof(aBuff));
    aFile.getline(aBuff, 4096);
    if (aBuff[0] == '\0')
    {
      continue;
    }

    if (IsActive(Counter::MemVirtual) && strncmp(aBuff, "VmSize:", strlen("VmSize:")) == 0)
    {
      myCounters[size_t(Counter::MemVirtual)] = atol(aBuff + strlen("VmSize:")) * 1024;
    }
    // else if (strncmp (aBuff, "VmPeak:", strlen ("VmPeak:")) == 0)
    //   myVirtualPeak = atol (aBuff + strlen ("VmPeak:")) * 1024;
    else if (IsActive(Counter::MemWorkingSet) && strncmp(aBuff, "VmRSS:", strlen("VmRSS:")) == 0)
    {
      // clang-format off
      myCounters[size_t(Counter::MemWorkingSet)] = atol (aBuff + strlen ("VmRSS:")) * 1024; // RSS - resident set size
    }
    else if (IsActive (Counter::MemWorkingSetPeak)
          && strncmp (aBuff, "VmHWM:", strlen ("VmHWM:")) == 0)
    {
      myCounters[size_t(Counter::MemWorkingSetPeak)] = atol (aBuff + strlen ("VmHWM:")) * 1024; // HWM - high water mark
      // clang-format on
    }
    else if (IsActive(Counter::MemPrivate) && strncmp(aBuff, "VmData:", strlen("VmData:")) == 0)
    {
      if (myCounters[size_t(Counter::MemPrivate)] == size_t(-1))
        ++myCounters[size_t(Counter::MemPrivate)];
      myCounters[size_t(Counter::MemPrivate)] += atol(aBuff + strlen("VmData:")) * 1024;
    }
    else if (IsActive(Counter::MemPrivate) && strncmp(aBuff, "VmStk:", strlen("VmStk:")) == 0)
    {
      if (myCounters[size_t(Counter::MemPrivate)] == size_t(-1))
        ++myCounters[size_t(Counter::MemPrivate)];
      myCounters[size_t(Counter::MemPrivate)] += atol(aBuff + strlen("VmStk:")) * 1024;
    }
  }
  aFile.close();
  #elif (defined(__APPLE__))
  if (IsActive(Counter::MemVirtual) || IsActive(Counter::MemWorkingSet)
      || IsActive(Counter::MemHeapUsage))
  {
    struct task_basic_info aTaskInfo;
    mach_msg_type_number_t aTaskInfoCount = TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&aTaskInfo, &aTaskInfoCount)
        == KERN_SUCCESS)
    {
      // On Mac OS X, these values in bytes, not pages!
      myCounters[size_t(Counter::MemVirtual)]    = aTaskInfo.virtual_size;
      myCounters[size_t(Counter::MemWorkingSet)] = aTaskInfo.resident_size;

      // Getting malloc statistics
      malloc_statistics_t aStats;
      malloc_zone_statistics(nullptr, &aStats);

      myCounters[size_t(Counter::MemHeapUsage)] = aStats.size_in_use;
    }
  }
  #endif
#endif
}

//=================================================================================================

TCollection_AsciiString OSD_MemInfo::ToString() const
{
  TCollection_AsciiString anInfo;
  if (hasValue(Counter::MemPrivate))
  {
    anInfo += TCollection_AsciiString("  Private memory:     ") + int(ValueMiB(Counter::MemPrivate))
              + " MiB\n";
  }
  if (hasValue(Counter::MemWorkingSet))
  {
    anInfo += TCollection_AsciiString("  Working Set:        ")
              + int(ValueMiB(Counter::MemWorkingSet)) + " MiB";
    if (hasValue(Counter::MemWorkingSetPeak))
    {
      anInfo +=
        TCollection_AsciiString(" (peak: ") + int(ValueMiB(Counter::MemWorkingSetPeak)) + " MiB)";
    }
    anInfo += "\n";
  }
  if (hasValue(Counter::MemSwapUsage))
  {
    anInfo += TCollection_AsciiString("  Pagefile usage:     ")
              + int(ValueMiB(Counter::MemSwapUsage)) + " MiB";
    if (hasValue(Counter::MemSwapUsagePeak))
    {
      anInfo +=
        TCollection_AsciiString(" (peak: ") + int(ValueMiB(Counter::MemSwapUsagePeak)) + " MiB)";
    }
    anInfo += "\n";
  }
  if (hasValue(Counter::MemVirtual))
  {
    anInfo += TCollection_AsciiString("  Virtual memory:     ") + int(ValueMiB(Counter::MemVirtual))
              + " MiB\n";
  }
  if (hasValue(Counter::MemHeapUsage))
  {
    anInfo += TCollection_AsciiString("  Heap memory:     ") + int(ValueMiB(Counter::MemHeapUsage))
              + " MiB\n";
  }
  return anInfo;
}

//=================================================================================================

size_t OSD_MemInfo::Value(const OSD_MemInfo::Counter theCounter) const
{
  if (theCounter >= Counter::MemCounter_NB || !IsActive(theCounter))
  {
    return size_t(-1);
  }
  return myCounters[size_t(theCounter)];
}

//=================================================================================================

size_t OSD_MemInfo::ValueMiB(const OSD_MemInfo::Counter theCounter) const
{
  if (theCounter >= Counter::MemCounter_NB || !IsActive(theCounter))
  {
    return size_t(-1);
  }
  return (myCounters[size_t(theCounter)] == size_t(-1))
           ? size_t(-1)
           : (myCounters[size_t(theCounter)] / (1024 * 1024));
}

//=================================================================================================

double OSD_MemInfo::ValuePreciseMiB(const OSD_MemInfo::Counter theCounter) const
{
  if (theCounter >= Counter::MemCounter_NB || !IsActive(theCounter))
  {
    return -1.0;
  }
  return (myCounters[size_t(theCounter)] == size_t(-1))
           ? -1.0
           : ((double)myCounters[size_t(theCounter)] / (1024.0 * 1024.0));
}

//=================================================================================================

TCollection_AsciiString OSD_MemInfo::PrintInfo()
{
  OSD_MemInfo anInfo;
  return anInfo.ToString();
}
