/************************************************************************* */
/**
 * HTTPBandwidthEstimator.cpp
 * @brief implementation of the HTTPBandwidthEstimator.
 *
 * COPYRIGHT 2011-2013 Qualcomm Technologies, Inc.
 * All rights reserved. Qualcomm Technologies proprietary and confidential.
 *
 ************************************************************************* */
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Source/IPStream/Source/HTTP/dev/DASH/src/HTTPBandwidthEstimator.cpp#10 $
$DateTime: 2013/08/14 12:00:06 $
$Change: 4274722 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* =======================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */

#include "HTTPBandwidthEstimator.h"
#include "SourceMemDebug.h"
#include "qtv_msg.h"

namespace video {

/**
 * c'tor
 */
HTTPBandwidthAggregate::HTTPBandwidthAggregate(uint32 timeToAddOnStart):
  m_pClock(NULL),
  m_PrevBucketIdx(-1),
  m_PrevBucketStartTime(-1),
  m_LastCalculatedBandwidth(-1),
  m_nBucketDuration(BUCKET_DURATION_MS)
{
  for (int i = 0; i < NUM_BUCKETS; ++i)
  {
    m_BucketArray[i].Reset();
  }
  m_CummulativeDataDownloaded = 0;
  m_CummulativeTimeElapsed = 0;
  m_TimeToAddOnStart = timeToAddOnStart;
  refCount = 0;
  timerRunning = FALSE;

  MM_CriticalSection_Create(&m_UpdateTimeLock);
  MM_CriticalSection_Create(&m_UpdateSizeLock);
}

/**
 * d'tor
 */
HTTPBandwidthAggregate::~HTTPBandwidthAggregate()
{
  if(m_UpdateTimeLock)
  {
    MM_CriticalSection_Release(m_UpdateTimeLock);
    m_UpdateTimeLock = NULL;
  }
  if(m_UpdateSizeLock)
  {
    MM_CriticalSection_Release(m_UpdateSizeLock);
    m_UpdateSizeLock = NULL;
  }
}

/**
 * Initialize with ptr to StreamSourceClock.
 */
bool HTTPBandwidthAggregate::Initialize(StreamSourceClock *pClock)
{
  m_pClock = pClock;
  return (m_pClock ? true : false);
}

/**
 * Set observation interval for BW estimation.
 */
void HTTPBandwidthAggregate::SetObservationInterval(const int nInterval)
{
  if (nInterval > 0)
  {
    // Set bucket duration based on the observation interval
    m_nBucketDuration = nInterval / (NUM_BUCKETS - 1);
  }
}

/**
 * Update a 'bucket' on a socket read.
 * If switching to a new bucket, then bandwidth is calculated
 * and cached.
 */
void HTTPBandwidthAggregate::Update(int32 bytesToRead, int32 bytesRead)
{
  if (m_pClock)
  {
    uint32 curTime = m_pClock->GetTickCount();
    uint32 deltaMs = curTime % m_nBucketDuration; // as bucket switch is every m_nBucketDuration.
    int currentBucketIdx = (curTime / m_nBucketDuration) % NUM_BUCKETS;

    Bucket& curBucket = m_BucketArray[currentBucketIdx];

    if (-1 == m_PrevBucketIdx)
    {
      // first time, if current time is close to start of bucket interval,
      // then enable updates on it, else disable all updates on it.
      if (deltaMs <= (uint32)THRESHOLD_ERROR_MS)
      {
        curBucket.Start();
        //QTV_MSG_PRIO1(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_MEDIUM,
        //  "HTTPBandwidthAggregate::Update() collect statistics for bucket %d",
        //  currentBucketIdx);
      }
      else
      {
        // don't call Start so all Updates to this bucket will be ignored.
        QTV_MSG_PRIO1(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_MEDIUM,
          "HTTPBandwidthAggregate::Update() first time ignore bucket %d",
          currentBucketIdx);
      }

      m_PrevBucketIdx = currentBucketIdx;
      m_PrevBucketStartTime = (int)curTime - (int)deltaMs;

      QTV_MSG_PRIO2(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_MEDIUM,
        "HTTPBandwidthAggregate::Update() Set startTime for bucket %d as %d",
        m_PrevBucketIdx, m_PrevBucketStartTime);
    }
    else
    {
      int updateTimeDiff = (int)(curTime - m_PrevBucketStartTime);

      if (updateTimeDiff >= m_nBucketDuration)
      {
        // switch to a new bucket.
        QTV_MSG_PRIO1(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_MEDIUM,
                      "HTTPBandwidthAggregate::Update() updateTimeDiff %d",
                      updateTimeDiff);

        int numBucketsToReset = 0;

        if (updateTimeDiff >= NUM_BUCKETS * m_nBucketDuration)
        {
          QTV_MSG_PRIO(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_MEDIUM,
            "HTTPBandwidthAggregate::Update() Reset all buckets");
          numBucketsToReset = NUM_BUCKETS;
        }
        else
        {
          if (updateTimeDiff > m_nBucketDuration + THRESHOLD_ERROR_MS)
          {
            numBucketsToReset = (int)((curTime - m_PrevBucketStartTime) / m_nBucketDuration);

            QTV_MSG_PRIO1(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_MEDIUM,
              "HTTPBandwidthAggregate::Update() Reset %d buckets",
              numBucketsToReset);
          }
        }

        if (numBucketsToReset > 0)
        {
          for (int i = 0; i < numBucketsToReset; ++i)
          {
            int bucketIdx = (m_PrevBucketIdx + i) % NUM_BUCKETS;
            QTV_MSG_PRIO1(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_MEDIUM,
              "HTTPBandwidthAggregate::Update() Reset bucket %d", bucketIdx);
            m_BucketArray[bucketIdx].Reset();
          }
        }
        else // zero buckets to reset.
        {
          QTV_MSG_PRIO(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_MEDIUM,
            "HTTPBandwidthAggregate::Update() Reset zero buckets");

          Bucket& prevBucket = m_BucketArray[m_PrevBucketIdx];
          int numBytesRequested = prevBucket.GetNumBytesRequested();

          if (numBytesRequested > 0)
          {
            prevBucket.Update(bytesToRead, bytesRead);

            QTV_MSG_PRIO6(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_MEDIUM,
              "HTTPBandwidthAggregate::Update() Bucket %d, done. Last %ld, %ld. Total %d, %ld, avg %f",
              m_PrevBucketIdx, bytesToRead, bytesRead, numBytesRequested, prevBucket.GetNumBytesRead(),
             (float)(1000.0 * numBytesRequested / (float)m_nBucketDuration));
          }
        }

        // Enable statistics to be collected in new bucket.
        curBucket.Start();

        QTV_MSG_PRIO2(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_MEDIUM,
          "HTTPBandwidthAggregate::Update() Set startTime for bucket %d as %d",
          m_PrevBucketIdx, m_PrevBucketStartTime);

        m_PrevBucketIdx = currentBucketIdx;
        m_PrevBucketStartTime = (int)curTime - (int)deltaMs;

        CalculateBandwidth(currentBucketIdx);
      }
      else
      {
        // continuing with previous bucket
        if (curBucket.GetNumBytesRequested() >= 0)
        {
          curBucket.Update(bytesToRead, bytesRead);

          //QTV_MSG_PRIO6(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_MEDIUM,
          //  "HTTPBandwidthAggregate::Update() Bucket %d: %d, %d. Total %d, %d, "
          //  "curTime %d", currentBucketIdx, bytesToRead, bytesRead,
          //  curBucket.GetNumBytesRequested(), curBucket.GetNumBytesRead(), (int)curTime);
        }
      }
    }
  }
  else
  {
    QTV_MSG_PRIO(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_ERROR,
                 "HTTPBandwidthAggregate::Update() pClock not initialized");
  }
}

/**
 * Returns the estimated bandwidth. If bandwidth is unknown,
 * then will use last calculated bandwidth. Returns '-1' when
 * not able to estimate the bandwidth.
 */
int HTTPBandwidthAggregate::GetEstimatedBandwidth() const
{
  return m_LastCalculatedBandwidth;
}

/**
 * Helper function to calculate the bandwidth when switching to
 * a new bucket. Takes a simple average from collected data.
 */
void HTTPBandwidthAggregate::CalculateBandwidth(int currentBucketIdx)
{
  int estdBw = 0;

  // calculate normalized weights for buckets that are in use.
  // The last bucket is not included as it is not completed.
  int sumRead = 0;
  int numBuckets = 0;
  for (int i = 0; i < NUM_BUCKETS - 1; ++i)
  {
    int bucketIdx = (i + currentBucketIdx + 1) % NUM_BUCKETS;
    Bucket& bucket = m_BucketArray[bucketIdx];
    int numBytesRequested = bucket.GetNumBytesRequested();

    if (numBytesRequested > 0)
    {
      sumRead += bucket.GetNumBytesRead();
      ++numBuckets;
    }
  }

  if (numBuckets > 0)
  {
    // calculate estiamted bw in bps.
    double numr = 8.0 * sumRead;
    double denr = (double)(numBuckets * m_nBucketDuration) / 1000.0;
    estdBw = (int)(numr / denr);
  }
  else
  {
    estdBw = -1;
  }

  // debugging.
  char buf[500];
  buf[0] = '\0';

  for (int i = 0; i < NUM_BUCKETS - 1; ++i)
  {
    char tmpStr[20];
    int bucketIdx = (i + currentBucketIdx + 1) % NUM_BUCKETS;
    Bucket& bucket = m_BucketArray[bucketIdx];

    std_strlprintf(tmpStr, sizeof(tmpStr),
                   "*(%d) [%ld,%ld] ", bucketIdx, bucket.GetNumBytesRequested(),
                   bucket.GetNumBytesRead());
    std_strlcat(buf, tmpStr, sizeof(buf));
  }

  QTV_MSG_SPRINTF_3(QTVDIAG_HTTP_STREAMING, "%s, bw %d, numBuckets %d", buf, (int)estdBw, numBuckets);

  if (estdBw >= 0)
  {
    m_LastCalculatedBandwidth = estdBw;
  }
}

/**
 * c'tor. Reset's the bucket.
 */
HTTPBandwidthAggregate::Bucket::Bucket()
{
  Reset();
}

/**
 * Needs to be called when a bucket needs to start collecting
 * statistics.
 */
HTTPBandwidthAggregate::Bucket::~Bucket()
{

}

/**
 * Bucket initialization. Also, should be called when a bucket
 * is invalidated, so furthur updates on the bucket will be
 * ignored.
 */
void HTTPBandwidthAggregate::Bucket::Reset()
{
  m_NumBytesRequested = -1;
  m_NumBytesRead = -1;
}

/**
 * Needs to be called when a bucket needs to start collecting
 * statistics.
 */
void HTTPBandwidthAggregate::Bucket::Start()
{
  m_NumBytesRequested = 0;
  m_NumBytesRead = 0;
}

/**
 * Update a bucket on a socket read. It will be updated is Start
 * was called on it, and there was not Reset after Start was
 * called.
 */
void HTTPBandwidthAggregate::Bucket::Update(int incNumRequested,
                                            int incNumRead)
{
  m_NumBytesRequested += incNumRequested;
  m_NumBytesRead += incNumRead;
}

/**
 * Return the number of bytes requested in this bucket for
 * socket reads.
 */
int32 HTTPBandwidthAggregate::Bucket::GetNumBytesRequested() const
{
  return m_NumBytesRequested;
}

/**
 * Return the number of bytes read in this bucket for socket
 * reads.
 */
int32 HTTPBandwidthAggregate::Bucket::GetNumBytesRead() const
{
  return m_NumBytesRead;
}


HTTPBandwidthEstimator::HTTPBandwidthEstimator() :
  HTTPBandwidthAggregate(TIME_TO_ADD_ON_START),
  m_eState(IDLE),
  m_NumOutstaningRequests(0)
{

}

HTTPBandwidthEstimator::~HTTPBandwidthEstimator()
{

}

void HTTPBandwidthEstimator::RequestSent()
{
  ++m_NumOutstaningRequests;
  QTV_MSG_PRIO1(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_LOW,
                "RequestSent: %u", m_NumOutstaningRequests);
}

void HTTPBandwidthEstimator::ResponseHeaderReceived()
{
  if (IDLE == m_eState)
  {
    QTV_MSG_PRIO1(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_LOW,
                  "ResponseHeaderReceived num %u", m_NumOutstaningRequests);
    m_eState = BUSY;
    StartTimer();
  }
}


void HTTPBandwidthEstimator::ResponseDataReceived()
{
  --m_NumOutstaningRequests;

  if (0 == m_NumOutstaningRequests && BUSY == m_eState)
  {
    StopTimer();
    m_eState = IDLE;
  }

  QTV_MSG_PRIO1(QTVDIAG_HTTP_STREAMING, QTVDIAG_PRIO_LOW,
               "ResponseDataReceived num %u", m_NumOutstaningRequests);
}

} // end namespace video
