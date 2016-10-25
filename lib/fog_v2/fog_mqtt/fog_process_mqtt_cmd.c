#include "fog_v2_config.h"
#include "fog_process_mqtt_cmd.h"


#if  (FOG_MQTT_DEBUG == 1)
#define app_log(M, ...)         custom_log("FOG_PROCESS_MQTT_CMD", M, ##__VA_ARGS__)
#else
#define app_log(M, ...)
#endif

OSStatus process_fog_v2_mqtt_cmd(const char *cmd_payload);

//处理fog下发的mqtt cmd消息
OSStatus process_fog_v2_mqtt_cmd(const char *cmd_payload)
{
    int32_t code = 0;
    OSStatus err = kGeneralErr;
    json_object *http_body_json_obj = NULL, *code_json_obj = NULL, *data_json_obj = NULL, *deviceid_json_obj = NULL;
    const char *cmd_deviceid = NULL;

#if (FOG_V2_USE_SUB_DEVICE == 1)
    uint32_t index = 0;
#endif

    require_string( cmd_payload != NULL, exit, "cmd_payload is NULL ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" );

    require_string( ((*cmd_payload == '{') && (*(cmd_payload + strlen( cmd_payload ) - 1) == '}')), exit, "http body JSON format error" );

    http_body_json_obj = json_tokener_parse( cmd_payload );
    if ( http_body_json_obj == NULL )
    {
        app_log("num_of_chunks:%d, free:%d", MicoGetMemoryInfo()->num_of_chunks, MicoGetMemoryInfo()->free_memory);
        app_log("cmd_payload:%s", cmd_payload);
        err = kGeneralErr;
        goto exit;
    }

    code_json_obj = json_object_object_get( http_body_json_obj, "code" );
    require_string( code_json_obj != NULL, exit, "get code error!" );

    code = json_object_get_int( code_json_obj );

    if(code == FOG_MQTT_CMD_UNBIND)
    {
        data_json_obj = json_object_object_get( http_body_json_obj, "data" );
        require_string( code_json_obj != NULL, exit, "get data error!" );

        deviceid_json_obj = json_object_object_get( data_json_obj, "deviceid" );
        require_string( code_json_obj != NULL, exit, "get deviceid error!" );

        cmd_deviceid = json_object_get_string(deviceid_json_obj); //字符串类型
        require_string(cmd_deviceid != NULL, exit, "get user_key_obj error!");

        if ( strcmp( cmd_deviceid, get_fog_des_g( )->device_id ) == 0 ) //先判断ID是否为父设备ID
        {
            get_fog_des_g( )->is_hava_superuser = false;
            mico_system_context_update( mico_system_context_get( ) );
            app_log("[NOTICE]bonjour set have no superuser!");

            stop_fog_bonjour( );
            start_fog_bonjour( false, get_fog_des_g( ) );   //开启bonjour

#if (FOG_V2_USE_SUB_DEVICE == 1)
            push_cmd_to_subdevice_queue(MQTT_CMD_GATEWAY_UNBIND, cmd_deviceid);//发送消息给队列
#endif
        } else //判断子设备ID
        {
#if (FOG_V2_USE_SUB_DEVICE == 1)
            if(get_sub_device_queue_index_by_deviceid(&index, cmd_deviceid) == true)
            {
                push_cmd_to_subdevice_queue(MQTT_CMD_SUB_UNBIND, cmd_deviceid);//发送消息给队列
            }else{
                app_log("[CMD]device id error, %s", cmd_deviceid);
            }
#endif
        }
    }else if(code == FOG_MQTT_CMD_BIND)
    {
        get_fog_des_g()->is_hava_superuser = true;
        mico_system_context_update(mico_system_context_get());

        app_log("[NOTICE]bonjour set have superuser!");

        stop_fog_bonjour();
        start_fog_bonjour(false, get_fog_des_g());   //开启bonjour

#if (FOG_V2_USE_SUB_DEVICE == 1)
        push_cmd_to_subdevice_queue(MQTT_CMD_GATEWAY_BIND, cmd_deviceid);
#endif
    }else if(code == FOG_MQTT_CMD_SUBDEVICE_WAIT_BIND)
    {
        app_log("[NOTICE]you can add subdevice now!!!");
    }else
    {
        app_log("cmd code error, code:%ld!!!", code);
    }

    err = kNoErr;
    exit:
    if ( http_body_json_obj != NULL )
    {
        json_object_put( http_body_json_obj );
        http_body_json_obj = NULL;
    }

    return err;
}


