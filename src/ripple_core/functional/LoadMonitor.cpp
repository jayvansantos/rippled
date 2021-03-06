//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================


SETUP_LOG (LoadMonitor)

LoadMonitor::LoadMonitor ()
    : mLock (this, "LoadMonitor", __FILE__, __LINE__)
    , mCounts (0)
    , mLatencyEvents (0)
    , mLatencyMSAvg (0)
    , mLatencyMSPeak (0)
    , mTargetLatencyAvg (0)
    , mTargetLatencyPk (0)
    , mLastUpdate (UptimeTimer::getInstance ().getElapsedSeconds ())
{
}

// VFALCO NOTE WHY do we need "the mutex?" This dependence on
//         a hidden global, especially a synchronization primitive,
//         is a flawed design.
//         It's not clear exactly which data needs to be protected.
//
// call with the mutex
void LoadMonitor::update ()
{
    int now = UptimeTimer::getInstance ().getElapsedSeconds ();

    // VFALCO TODO stop returning from the middle of functions.

    if (now == mLastUpdate) // current
        return;

    // VFALCO TODO Why 8?
    if ((now < mLastUpdate) || (now > (mLastUpdate + 8)))
    {
        // way out of date
        mCounts = 0;
        mLatencyEvents = 0;
        mLatencyMSAvg = 0;
        mLatencyMSPeak = 0;
        mLastUpdate = now;
        // VFALCO TODO don't return from the middle...
        return;
    }

    // do exponential decay
    /*
        David:
        
        "Imagine if you add 10 to something every second. And you
         also reduce it by 1/4 every second. It will "idle" at 40,
         correponding to 10 counts per second."
    */
    do
    {
        ++mLastUpdate;
        mCounts -= ((mCounts + 3) / 4);
        mLatencyEvents -= ((mLatencyEvents + 3) / 4);
        mLatencyMSAvg -= (mLatencyMSAvg / 4);
        mLatencyMSPeak -= (mLatencyMSPeak / 4);
    }
    while (mLastUpdate < now);
}

void LoadMonitor::addCount ()
{
    ScopedLockType sl (mLock, __FILE__, __LINE__);

    update ();
    ++mCounts;
}

void LoadMonitor::addLatency (int latency)
{
    // VFALCO NOTE Why does 1 become 0?
    if (latency == 1)
        latency = 0;

    ScopedLockType sl (mLock, __FILE__, __LINE__);

    update ();

    ++mLatencyEvents;
    mLatencyMSAvg += latency;
    mLatencyMSPeak += latency;

    // VFALCO NOTE Why are we multiplying by 4?
    int const latencyPeak = mLatencyEvents * latency * 4;

    if (mLatencyMSPeak < latencyPeak)
        mLatencyMSPeak = latencyPeak;
}

void LoadMonitor::addLoadSample (LoadEvent const& sample)
{
    std::string const& name (sample.name());
    std::size_t latency (sample.getSecondsTotal());

    if (latency > 500)
    {
        WriteLog ((latency > 1000) ? lsWARNING : lsINFO, LoadMonitor)
            << "Job: " << name << " ExecutionTime: " << sample.getSecondsRunning() <<
            " WaitingTime: " << sample.getSecondsWaiting();
    }

    // VFALCO NOTE Why does 1 become 0?
    if (latency == 1)
        latency = 0;

    ScopedLockType sl (mLock, __FILE__, __LINE__);

    update ();
    ++mCounts;
    ++mLatencyEvents;
    mLatencyMSAvg += latency;
    mLatencyMSPeak += latency;

    // VFALCO NOTE Why are we multiplying by 4?
    int const latencyPeak = mLatencyEvents * latency * 4;

    if (mLatencyMSPeak < latencyPeak)
        mLatencyMSPeak = latencyPeak;
}

void LoadMonitor::setTargetLatency (uint64 avg, uint64 pk)
{
    mTargetLatencyAvg  = avg;
    mTargetLatencyPk = pk;
}

bool LoadMonitor::isOverTarget (uint64 avg, uint64 peak)
{
    return (mTargetLatencyPk && (peak > mTargetLatencyPk)) ||
           (mTargetLatencyAvg && (avg > mTargetLatencyAvg));
}

bool LoadMonitor::isOver ()
{
    ScopedLockType sl (mLock, __FILE__, __LINE__);

    update ();

    if (mLatencyEvents == 0)
        return 0;

    return isOverTarget (mLatencyMSAvg / (mLatencyEvents * 4), mLatencyMSPeak / (mLatencyEvents * 4));
}

void LoadMonitor::getCountAndLatency (uint64& count, uint64& latencyAvg, uint64& latencyPeak, bool& isOver)
{
    ScopedLockType sl (mLock, __FILE__, __LINE__);

    update ();

    count = mCounts / 4;

    if (mLatencyEvents == 0)
    {
        latencyAvg = 0;
        latencyPeak = 0;
    }
    else
    {
        latencyAvg = mLatencyMSAvg / (mLatencyEvents * 4);
        latencyPeak = mLatencyMSPeak / (mLatencyEvents * 4);
    }

    isOver = isOverTarget (latencyAvg, latencyPeak);
}

// vim:ts=4
