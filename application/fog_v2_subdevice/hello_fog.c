#include "config/fog_v2_config.h"
#include "mico.h"
#include "user_common.h"
#include "fogcloud_up_thread.h"
#include "fogcloud_down_thread.h"
#include "micokit_ext.h"
#include "fog_v2_subdevice_app.h"

#define app_log(M, ...)             custom_log("APP", M, ##__VA_ARGS__)
#define app_log_trace()             custom_log_trace("APP")

static mico_semaphore_t wifi_sem;

void appNotify_WifiStatusHandler( WiFiEvent status, void* const inContext )
{
    switch ( status )
    {
        case NOTIFY_STATION_UP:
            app_log("Wi-Fi connected.");
            mico_rtos_set_semaphore( &wifi_sem );
            break;
        case NOTIFY_STATION_DOWN:
            app_log("Wi-Fi disconnected.");
            break;
        default:
            break;
    }
}

int application_start(void)
{
	app_log_trace();
	OSStatus err = kNoErr;
    mico_Context_t* mico_context;

    app_log("APP Version:%s%s", FOG_V2_REPORT_VER, FOG_V2_REPORT_VER_NUM);

    micokit_ext_set();

    err = mico_rtos_init_semaphore( &wifi_sem, 1 );
    require_noerr( err, exit );

    /* Register user function for MiCO notification: WiFi status changed */
    err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *) appNotify_WifiStatusHandler, NULL );
    require_noerr( err, exit );

	mico_context = mico_system_context_init(sizeof(FOG_DES_S));

	err = mico_system_init(mico_context);
	require_noerr( err, exit);

    /* wait for wifi on */
    mico_rtos_get_semaphore( &wifi_sem, MICO_WAIT_FOREVER );

    err = start_fog_v2_service();
    require_noerr( err, exit);

    fog_sub_api_test();

    /*Start fog downstream thread*/
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "fogcloud_down", user_downstream_thread, 0x1000, (uint32_t)NULL);
    require_noerr_string( err, exit, "ERROR: Unable to start the fogclod downstream thread." );

    /* Start fog upstream thread */
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "fogcloud_up", user_upstream_thread, 0x1000, (uint32_t)NULL);
    require_noerr_string( err, exit, "ERROR: Unable to start the fogclod upstream thread." );

	exit: mico_rtos_delete_thread(NULL);
	return err;
}
