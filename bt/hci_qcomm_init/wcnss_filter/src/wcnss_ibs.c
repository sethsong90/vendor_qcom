/*==========================================================================
Description
  Wcnss_ibs has the state-machine and logic to process sleep bytes to excercise
  SIBS (Software In-band sleep) protocol

# Copyright (c) 2013 by Qualcomm Technologies, Inc.  All Rights Reserved.
# Qualcomm Technologies Proprietary and Confidential.

===========================================================================*/
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <cutils/log.h>
#include <pthread.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include "wcnss_ibs.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/
#define DEFAULT_LPM_IDLE_TIMEOUT 1000
#define true 1
#define false 0
/********************IBS MACROS and State-Machine ****************************/
#define TX_IDLE_DELAY           10
#define LOG_TAG "WCNSS_IBSLEEP"

#define USERIAL_OP_CLK_ON 13
#define USERIAL_OP_CLK_OFF 14

/* HCI_IBS transmit side sleep protocol states */
typedef enum {
    IBS_TX_ASLEEP = 0,
    IBS_TX_WAKING,
    IBS_TX_AWAKE,
}tx_ibs_states;

/* HCI_IBS receive side sleep protocol states */
typedef enum {
    IBS_RX_ASLEEP = 0,
    IBS_RX_AWAKE,
}rx_ibs_states;

typedef enum {
    HCI_IBS_TX_VOTE_CLOCK_ON = 0,
    HCI_IBS_TX_VOTE_CLOCK_OFF,
    HCI_IBS_RX_VOTE_CLOCK_ON,
    HCI_IBS_RX_VOTE_CLOCK_OFF,
} hci_ibs_clock_state_vote;

typedef struct{

    pthread_mutex_t wack_lock;
    pthread_mutex_t hci_ibs_lock;
    uint8_t tx_ibs_state;
    uint8_t tx_vote;
    uint8_t rx_ibs_state;
    uint8_t rx_vote;

    uint8_t timer_created;
    timer_t timer_id;
    uint32_t timeout_ms;

}IBS_STATE_MACHINE;
IBS_STATE_MACHINE ibs_state_machine;

static volatile unsigned char wack_recvd = 0;
pthread_cond_t wack_cond = PTHREAD_COND_INITIALIZER;


void ibs_start_wack_timer(void);
void ibs_stop_wack_timer(void);
/********************IBS MACROS and State-Machine ****************************/

/********************Low power mode state ************************************/
/* LPM WAKE state */
enum {
    WCNSS_IBS_WAKE_DEASSERTED = 0,              /* initial state */
    WCNSS_IBS_WAKE_W4_TX_DONE,
    WCNSS_IBS_WAKE_W4_TIMEOUT,
    WCNSS_IBS_WAKE_ASSERTED
};
/* low power mode control block */
typedef struct
{
    //uint8_t state;                          /* Low power mode state */
    uint8_t wake_state;                     /* LPM WAKE state */
    uint8_t no_tx_data;
    uint8_t timer_created;
    timer_t timer_id;
    uint32_t timeout_ms;
}bt_lpm;
static bt_lpm wcnss_bt_lpm;
/********************Low power mode state ************************************/
static void ibs_wake_retrans_timeout(void);
static void ibs_wack_timer_start(void);
static void ibs_wack_timer_stop(void);

int tty_fd=-1;
static int num_try;
static void do_write(uint8_t ibs_data)
{
    int ret;
    pthread_mutex_lock(&signal_mutex);
    ret = write(tty_fd,&ibs_data,1);
    if(ret < 0)
    {
        ALOGE("%s: Failed to write",__FUNCTION__);
    }
    pthread_mutex_unlock(&signal_mutex);
}

static void ibs_wake_retrans_timeout(void)
{
    uint8_t ibs_data;
    ALOGV("%s: Retransmit wake ind and restart timer",__FUNCTION__);
    if(num_try == 20)
    {
        ibs_wack_timer_stop();
        wack_recvd = false;
        return;
    }
    ibs_wack_timer_start();
    ibs_data = IBS_WAKE_IND;
    ALOGV("%s: Writing HCI_IBS_WAKE_IND",__FUNCTION__);
    if(tty_fd > 0)
        write(tty_fd,&ibs_data,1);
    else
    {
        ALOGE("%s: Invalid FD",__FUNCTION__);
        return;
    }
    num_try++;
}

static void ibs_wack_timer_stop()
{
    int status;
    struct itimerspec ts;
    ALOGV("%s",__FUNCTION__);
    num_try = 0;
    if(ibs_state_machine.timer_created == true)
    {
        ts.it_value.tv_sec = 0;
        ts.it_value.tv_nsec = 0;
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;
        status = timer_settime(ibs_state_machine.timer_id, 0, &ts, 0);
        if(status == -1)
            ALOGE("%s:Failed to stop set wack timer",__FUNCTION__);
        else if(status == 0)
            ALOGE("%s: Wack timer Stopped",__FUNCTION__);
     }
}

void ibs_msm_serial_clock_vote(uint8_t vote, IBS_STATE_MACHINE * ibs_state)
{
    uint8_t new_vote;
    uint8_t old_vote = (ibs_state->tx_vote | ibs_state->rx_vote);
    ALOGV("%s", __FUNCTION__);

    switch (vote)
    {
    case HCI_IBS_TX_VOTE_CLOCK_ON:
        ibs_state->tx_vote = 1;
        new_vote = 1;
        break;
    case HCI_IBS_RX_VOTE_CLOCK_ON:
        ibs_state->rx_vote = 1;
        new_vote = 1;
        break;
    case HCI_IBS_TX_VOTE_CLOCK_OFF:
        ibs_state->tx_vote = 0;
        new_vote = ibs_state->rx_vote | ibs_state->tx_vote;
        break;
    case HCI_IBS_RX_VOTE_CLOCK_OFF:
        ibs_state->rx_vote = 0;
        new_vote = ibs_state->rx_vote | ibs_state->tx_vote;
        break;
    default:
        ALOGE("ibs_msm_serial_clock_vote: Wrong vote requested!\n");
        return;
    }
    ALOGV("new_vote: (%d) ## old-vote: (%d)", new_vote, old_vote);

    if (new_vote != old_vote) {
        if (new_vote) {
            /*vote UART CLK ON using UART driver's ioctl() */
            ALOGE("%s: vote UART CLK ON using UART driver's ioctl()",
                __FUNCTION__);
            ioctl(tty_fd,USERIAL_OP_CLK_ON);
        }
        else {
            /*vote UART CLK OFF using UART driver's ioctl() */
            ALOGE("%s: vote UART CLK OFF using UART driver's ioctl()",
                __FUNCTION__);
            ioctl(tty_fd,USERIAL_OP_CLK_OFF);
        }
    }
}

static void ibs_wack_timer_start()
{
    int status;
    struct itimerspec ts;
    struct sigevent se;
    ALOGV("%s",__FUNCTION__);
    if(ibs_state_machine.timer_created == false)
    {
    se.sigev_notify_function = ibs_wake_retrans_timeout;
        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = &ibs_state_machine.timer_id;
        se.sigev_notify_attributes = NULL;

        status = timer_create(CLOCK_MONOTONIC, &se, &ibs_state_machine.timer_id);
        if (status == 0)
        {
            ALOGV("%s: WACK timer created",__FUNCTION__);
            ibs_state_machine.timer_created = true;
        }
    }
    if(ibs_state_machine.timer_created == true)
    {
        if(!ibs_state_machine.timeout_ms)
        ibs_state_machine.timeout_ms = TX_IDLE_DELAY;
        ts.it_value.tv_sec = ibs_state_machine.timeout_ms/1000;
        ts.it_value.tv_nsec = 1000000*(ibs_state_machine.timeout_ms%1000);
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;

        status = timer_settime(ibs_state_machine.timer_id, 0, &ts, 0);
        if (status == -1)
            ALOGE("%s:Failed to set wack timer",__FUNCTION__);
    }
}

static int ibs_bt_device_wakeup()
{
    uint8_t ibs_data;
    int status = 0;
    ALOGV("%s", __FUNCTION__);
    switch(ibs_state_machine.tx_ibs_state)
    {
    case IBS_TX_ASLEEP:
        ALOGD("%s: TX state ASLEEP, acquire SM lock",__FUNCTION__);
        status = pthread_mutex_trylock(&ibs_state_machine.hci_ibs_lock);
        if(status < 0)
        {
            ALOGE("%s: Failed to Acquire SM lock- Returning",__FUNCTION__);
            return -1;
        }
        ibs_state_machine.tx_ibs_state = IBS_TX_WAKING;
        /*************************/
        //TODO Vote-on UART//
        /*************************/
        ALOGV("%s: UART TX Vote ON",__FUNCTION__);
        //ibs_msm_serial_clock_vote(IBS_TX_VOTE_CLOCK_ON,&ibs_state_machine);
        ALOGV("%s: write wake ind",__FUNCTION__);
        ibs_data = IBS_WAKE_IND;
        ALOGD("%s: Writing IBS_WAKE_IND",__FUNCTION__);
        if(tty_fd > 0)
            do_write(ibs_data);
        else
        {
            ALOGE("%s: Invalid FD",__FUNCTION__);
            return -1;
        }
        ALOGV("%s: Acquire wake lock",__FUNCTION__);
        pthread_mutex_lock(&ibs_state_machine.wack_lock);

        ibs_wack_timer_start();
        wack_recvd = false;
        while(wack_recvd == false)
        {
            pthread_cond_wait(&wack_cond, &ibs_state_machine.wack_lock);
            ALOGV("%s: Get wake ack from the SOC",__FUNCTION__);
        }
        if(wack_recvd == true)
        {
            ALOGD("%s: Received wack ack",__FUNCTION__);
            ALOGD("%s: TX state transition to AWAKE",__FUNCTION__);
            ibs_state_machine.tx_ibs_state = IBS_TX_AWAKE;
            wack_recvd = false;
            status = 0;
        }
        else
        {
            ALOGE("%s: Failed to wake SOC",__FUNCTION__);
            ibs_wack_timer_stop();
            wack_recvd = false;
            status = -1;
        }
        ALOGV("%s: Realease wake lock and SM lock",__FUNCTION__);
        pthread_mutex_unlock(&ibs_state_machine.wack_lock);
        pthread_mutex_unlock(&ibs_state_machine.hci_ibs_lock);
        break;
    case IBS_TX_WAKING:
        ALOGV("%s: SOC is WAKING UP",__FUNCTION__);
        break;
    case IBS_TX_AWAKE:
        ALOGD("%s: SOC is already AWAKE",__FUNCTION__);
        break;
    }
    return status;
}

void ibs_recv_ibs_cmd(uint8_t *data)
{
    uint8_t ibs_data;
    int status = 0;
    ALOGV("%s",__FUNCTION__);
    switch(*data)
    {
    case IBS_WAKE_IND: //0xFD
        ALOGD("%s: Received IBS_WAKE_IND: 0xFD",__FUNCTION__);
        status = pthread_mutex_trylock(&ibs_state_machine.hci_ibs_lock);
        if(status < 0)
        {
            ALOGD("%s: Failed to Acquire SM lock- Returning",__FUNCTION__);
            return;
        }
        switch(ibs_state_machine.rx_ibs_state)
        {
        case IBS_RX_ASLEEP:
            ibs_state_machine.rx_ibs_state = IBS_RX_AWAKE;
            ibs_data = IBS_WAKE_ACK;
            ALOGV("%s: Writing IBS_WAKE_ACK",__FUNCTION__);

            if(tty_fd > 0)
            {
                ALOGV("%s: Lock to write",__FUNCTION__);
                do_write(ibs_data);
            }
            else
            {
                ALOGE("%s: Invalid FD",__FUNCTION__);
                return;
            }
            /*************************/
            //TODO Vote-on UART//
            //vote_uart_clk(ON);
            /*************************/
            ALOGV("%s: UART RX Vote ON",__FUNCTION__);
            //ibs_msm_serial_clock_vote(IBS_RX_VOTE_CLOCK_ON,&ibs_state_machine);
            break;
        case IBS_RX_AWAKE:
            ibs_data = IBS_WAKE_ACK;
            ALOGV("%s: Writing IBS_WAKE_ACK",__FUNCTION__);
            if(tty_fd > 0 )
                do_write(ibs_data);
            else
            {
                ALOGE("%s: Invalid FD",__FUNCTION__);
                return;
            }
            break;
        }
        pthread_mutex_unlock(&ibs_state_machine.hci_ibs_lock);
        break;
    case IBS_SLEEP_IND: //0xFE
        ALOGD("%s: Received IBS_SLEEP_IND: 0xFE",__FUNCTION__);
        status = pthread_mutex_trylock(&ibs_state_machine.hci_ibs_lock);
        if(status < 0)
        {
            ALOGD("%s: Failed to Acquire SM lock- Returning",__FUNCTION__);
            return;
        }
        switch(ibs_state_machine.rx_ibs_state)
        {
        case IBS_RX_AWAKE:
            ALOGV("%s: RX path is awake, Vote Off uart",__FUNCTION__);
            ibs_state_machine.rx_ibs_state = IBS_RX_ASLEEP;
            /*************************/
            //TODO Vote-off UART//
            /*************************/
            ALOGV("%s: UART RX Vote Off",__FUNCTION__);
            //ibs_msm_serial_clock_vote(IBS_RX_VOTE_CLOCK_OFF,&ibs_state_machine);
            break;
        case IBS_RX_ASLEEP:
            ALOGD("%s: RX path is asleep",__FUNCTION__);
            break;
        }
        pthread_mutex_unlock(&ibs_state_machine.hci_ibs_lock);
        break;
    case IBS_WAKE_ACK: ////0xFC
        ALOGD("%s: Received IBS_WAKE_ACK: 0xFC",__FUNCTION__);
        status = pthread_mutex_trylock(&ibs_state_machine.hci_ibs_lock);
        if(status < 0)
        {
            ALOGD("%s: Failed to Acquire SM lock- Returning",__FUNCTION__);
            return;
        }
        switch(ibs_state_machine.tx_ibs_state)
        {
        case IBS_TX_WAKING:
            ALOGD("%s: Signal wack_cond",__FUNCTION__);
            wack_recvd = true;
            ibs_wack_timer_stop();
            pthread_cond_signal(&wack_cond);
            break;
        case IBS_TX_AWAKE:
            ALOGD("%s: TX SM is awake already, stop wack timer if running",__FUNCTION__);
            ibs_wack_timer_stop();
            break;
        default:
            ALOGE("%s: WAKE ACK from SOC, Unexpected TX state",__FUNCTION__);
            break;
        }
        pthread_mutex_unlock(&ibs_state_machine.hci_ibs_lock);
        break;
    }
}
static void ibs_wcnss_bt_device_sleep(void)
{
    uint8_t ibs_data;
    int status = 0;
    ALOGV("%s: Acquire SM lock",__FUNCTION__);
    status = pthread_mutex_trylock(&ibs_state_machine.hci_ibs_lock);
    if(status < 0)
    {
         ALOGD("%s: Failed to Acquire SM lock- Returning",__FUNCTION__);
         return;
    }
    switch(ibs_state_machine.tx_ibs_state)
    {
    case IBS_TX_AWAKE:
        ALOGD("%s: TX Awake, Sending SLEEP_IND",__FUNCTION__);
        ibs_data = IBS_SLEEP_IND;

        if(tty_fd > 0)
            do_write(ibs_data);
        else
        {
            ALOGE("%s: Invalid FD",__FUNCTION__);
            return;
        }
        ibs_state_machine.tx_ibs_state    = IBS_TX_ASLEEP;
        /**********************/
        /*TODO: UART Vote Off */
        /**********************/
        ALOGV("%s: UART TX Vote Off",__FUNCTION__);
        //ibs_msm_serial_clock_vote(IBS_TX_VOTE_CLOCK_OFF,&ibs_state_machine);
        break;
    case IBS_TX_ASLEEP:
        ALOGD("%s: TX Asleep",__FUNCTION__);
        break;
    default:
        ALOGE("%s: Invalid TX SM",__FUNCTION__);
        break;
    }
    pthread_mutex_unlock(&ibs_state_machine.hci_ibs_lock);
    return;
}

static void wcnss_wake_deassert(void)
{
    //wcnss_bt_lpm.wake_state = WCNSS_IBS_WAKE_DEASSERTED;
    ALOGV("%s",__FUNCTION__);
    ibs_wcnss_bt_device_sleep();
}

static void wcnss_idle_timeout(void)
{
    ALOGV("%s: Deassert SOC",__FUNCTION__);
    wcnss_wake_deassert();
}

static void wcnss_start_idle_timeout_timer(void)
{
    int status;
    struct itimerspec ts;
    struct sigevent se;
    ALOGV("%s",__FUNCTION__);
    if (wcnss_bt_lpm.timer_created == false)
    {
        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = &wcnss_bt_lpm.timer_id;
        se.sigev_notify_function = wcnss_idle_timeout;
        se.sigev_notify_attributes = NULL;

        status = timer_create(CLOCK_MONOTONIC, &se, &wcnss_bt_lpm.timer_id);

        if (status == 0)
            wcnss_bt_lpm.timer_created = true;
    }

    if (wcnss_bt_lpm.timer_created == true)
    {
        ts.it_value.tv_sec = wcnss_bt_lpm.timeout_ms/1000;
        ts.it_value.tv_nsec = 1000*(wcnss_bt_lpm.timeout_ms%1000);
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;

        status = timer_settime(wcnss_bt_lpm.timer_id, 0, &ts, 0);
        if (status == -1)
            ALOGE("%s: Failed to set LPM idle timeout",__FUNCTION__);
    }
}

static void wcnss_stop_idle_timeout_timer(void)
{
    int status;
    struct itimerspec ts;
    ALOGV("%s",__FUNCTION__);
    if (wcnss_bt_lpm.timer_created == true)
    {
        ts.it_value.tv_sec = 0;
        ts.it_value.tv_nsec = 0;
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;

        status = timer_settime(wcnss_bt_lpm.timer_id, 0, &ts, 0);
        if (status == -1)
            ALOGE("[STOP] Failed to set LPM idle timeout");

    }
}


void wcnss_ibs_init(int fd)
{
    ALOGV("%s",__FUNCTION__);
    memset(&wcnss_bt_lpm, 0, sizeof(wcnss_bt_lpm));
    wcnss_bt_lpm.timeout_ms = DEFAULT_LPM_IDLE_TIMEOUT;
    tty_fd = fd;
    ALOGD("%s: tty_fd = %d",__FUNCTION__,tty_fd);
}

void wcnss_tx_done(uint8_t tx_done)
{
    ALOGV("%s",__FUNCTION__);
    wcnss_bt_lpm.no_tx_data = tx_done;
    if(tx_done == true)
    {
       ALOGD("%s: tx_done. Start idle timeout",__FUNCTION__);
       wcnss_start_idle_timeout_timer();
    }
}

void wcnss_wake_assert(void)
{
    int ret;
    ALOGV("%s",__FUNCTION__);
    wcnss_stop_idle_timeout_timer();
    //wcnss_bt_lpm.wake_state = WCNSS_IBS_WAKE_ASSERTED;
    ibs_bt_device_wakeup();
    wcnss_tx_done(false);
}

void wcnss_device_can_sleep()
{
    if(wcnss_bt_lpm.wake_state == WCNSS_IBS_WAKE_ASSERTED)
    {
        if(wcnss_bt_lpm.no_tx_data == true)
        {
            ALOGD("%s: Start idle timeout",__FUNCTION__);
            wcnss_bt_lpm.wake_state == WCNSS_IBS_WAKE_W4_TIMEOUT;
            wcnss_start_idle_timeout_timer();
        }
        else
        {
            ALOGD("%s: tx not done yet",__FUNCTION__);
            wcnss_bt_lpm.wake_state == WCNSS_IBS_WAKE_W4_TX_DONE;
        }
    }
}

void wcnss_ibs_cleanup()
{
    ALOGV("%s",__FUNCTION__);
    if (wcnss_bt_lpm.timer_created == true)
    {
        timer_delete(wcnss_bt_lpm.timer_id);
    }
    if(ibs_state_machine.timer_created == true)
    {
       timer_delete(ibs_state_machine.timer_id);
    }
    tty_fd = -1;
}
