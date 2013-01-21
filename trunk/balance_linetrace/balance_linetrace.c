#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"
#include "balancer.h"
#include "port_interface.h"
#include "bluetooth_interface.h"

//#define GYRO_OFFSET	610	/* ジャイロセンサオフセット値(角速度0[deg/sec]時) */
//#define WHITE	530		/* 白色の光センサ値 */
//#define BLACK	720		/* 黒色の光センサ値 */
#define DELTA_T		0.004
#define KP		0.0	/* 比例ゲイン */
#define KI		0.0	/* 積分ゲイン */
#define KD		0.0	/* 微分ゲイン */

//*****************************************************************************
// 関数名：ecrobot_device_initialize
// 引数：なし
// 戻り値：なし
// 概要：ECROBOTデバイス初期化処理フック関数
//*****************************************************************************
void ecrobot_device_initialize(){				// OSEK起動時の処理（モータ停止）
	nxt_motor_set_speed(LEFT_MOTOR,0,0);			// nxt_motor_speed(n,int speed_persent,brake)
	nxt_motor_set_speed(RIGHT_MOTOR,0,0);			// n : NXT_PORT_A , NXT_PORT_B(右) , NXT_PORT_C(左)
//	nxt_motor_set_speed(TAIL_MOTOR,50,0);			// speed_persent : -100 〜 100
//	systick_wait_ms(10);					// brake : 0(フロートモード) , 1(ブレーキモード)
//	nxt_motor_set_speed(TAIL_MOTOR,0,1);

	nxt_motor_set_count(TAIL_MOTOR,0);
	ecrobot_set_light_sensor_active(LIGHT_SENSOR);		/* ライトセンサ点灯 */
	ecrobot_init_sonar_sensor(SONAR_SENSOR);		/*sonar*/
	ecrobot_init_bt_slave(BT_PASS_KEY);			/* スレーブとして初期化 */

}

//*****************************************************************************
// 関数名：ecrobot_device_terminate
// 引数：なし
// 戻り値：なし
// 概要：ECROBOTデバイス終了処理フック関数
//*****************************************************************************
void ecrobot_device_terminate(){				// OSEK終了時の処理（モータ停止）
	display_string("DT");
	display_update();	
	nxt_motor_set_speed(LEFT_MOTOR,0,0);
	nxt_motor_set_speed(RIGHT_MOTOR,0,0);
	ecrobot_term_bt_connection();				/* bluetooth通信終了 */
	ecrobot_set_light_sensor_inactive(LIGHT_SENSOR);	/* ライトセンサ消灯 */
	ecrobot_term_sonar_sensor(SONAR_SENSOR);		/*sonar*/
}

//*****************************************************************************
// 関数名：user_1ms_isr_type2
// 引数：なし
// 戻り値：なし
// 概要：1msec周期割り込みフック関数(OSEK ISR type2カテゴリ)
//*****************************************************************************
void user_1ms_isr_type2(void){}


//*****************************************************************************
// 関数名：calibration
// 引数：black,white,gray
// 戻り値：観測値
// 概要：光センサによる色彩値設定
//*****************************************************************************
void calibration(int *black,int *white,int *gray){			/* 閾値設定用ユーザ関数 */

	while( ecrobot_get_touch_sensor(TOUCH_SENSOR) == 0 ){		/* タッチセンサが押されていなかったら */

		*white = ecrobot_get_light_sensor(LIGHT_SENSOR);	/* 白の明るさ読み取り */

		display_clear(0);					/* 画面表示 */
		display_goto_xy(0, 1);
		display_string("WHITE=");
		display_int(*white, 4);
		display_update();

		systick_wait_ms(10);

	}

	systick_wait_ms(1000);


	while(ecrobot_get_touch_sensor(TOUCH_SENSOR) == 0){		/* タッチセンサが押されていなかったら */

		*black = ecrobot_get_light_sensor(LIGHT_SENSOR);	/* 黒の明るさ読み取り */

		display_clear(0);					/* 画面表示 */
		display_goto_xy(0, 1);
		display_string("BLACK=");
		display_int(*black, 4);
		display_update();

		systick_wait_ms(10);

	}

	systick_wait_ms(1000);

	*gray=( *black + *white ) / 2;					/* 中間値算出 */

	display_clear(0);
	display_goto_xy(0, 1);
	display_string("gray=");
	display_int(*gray, 4);
	display_update();

	systick_wait_ms(1000);

}



//*****************************************************************************
// タスク名 : TaskMain
// 概要 : メインタスク
//*****************************************************************************
TASK(TaskMain)
{
//	signed char forward;     	 			/* 前後進命令 */
	signed char turn;        	 			/* 旋回命令 */
	signed char pwm_L, pwm_R;				/* 左右モータPWM出力 */

	int sensorvalue;					/* ライトセンサ値を格納する変数を定義 */
//	float p_gain=1.0;					/* Pゲイン変数 */
//	float i_gain = 1.0;					/* Iゲイン変数 */
//	float d_gain=1.0;					/* Dゲイン変数 */
	int black, white, gray;					/* 色変数 */
	int light, light_tmp;					/* 1つ前の明るさを格納するlight_tmpを定義 */
	int before=0, standard=0;				/* before：1つ前の偏差、standard：現在の偏差 */
	float integral=0;
	int start_time,current_time;
	float off;


	balance_init();						/* 倒立振子制御初期化 */
	nxt_motor_set_count(LEFT_MOTOR, 0);			/* 左モータエンコーダリセット */
	nxt_motor_set_count(RIGHT_MOTOR, 0);			/* 右モータエンコーダリセット */

	/* キャリブレーション */
	calibration( &black, &white, &gray );			/* 閾値設定用関数呼び出し */

	systick_wait_ms(2000);					/* 2秒待って */
	nxt_motor_set_speed(TAIL_MOTOR,15,1);			/* 尻尾を下ろす */

	/* bluetoothセクション */
	while(1){

		ecrobot_read_bt_packet(bt_receive_buf, BT_RCV_BUF_SIZE);

		int ope = bt_receive_buf[0];
		if(ope==255){
			break;
		}else if(ope==101){
			AUTO_TAIL_POWER = bt_receive_buf[1];
			
		}else if(ope == 102){
			FORWARD_POWER = bt_receive_buf[1];
			
		}else if(ope == 103){
			BACK_POWER = (-1)*bt_receive_buf[1];
			
		}else if(ope == 150){
			ANGLE_TAIL = (int)bt_receive_buf[1];
			
		}else if(ope == 104){
			FORWARD_TIME = 1000*bt_receive_buf[1];
			
		}else if(ope == 105){
			 GYRO_OFFSET = 610+bt_receive_buf[1];
			
		}else if(ope == 106){
			 P_GAIN_TMP = bt_receive_buf[1];
			
		}else if(ope == 107){
			 I_GAIN_TMP = bt_receive_buf[1];
			
		}else if(ope == 108){
			 D_GAIN_TMP = bt_receive_buf[1];
			
		}

	}

	nxt_motor_set_speed(TAIL_MOTOR,-15,0);						/* 尻尾を上げてスタート */

	float p_gain = P_GAIN_TMP / 100;
	float i_gain = I_GAIN_TMP / 50;
	float d_gain = D_GAIN_TMP / 50;

	while(1)
	{

		sensorvalue = ecrobot_get_light_sensor(LIGHT_SENSOR);			/* ライトセンサ値を変数に代入 */

		light_tmp = ecrobot_get_light_sensor(LIGHT_SENSOR);

  		display_goto_xy(0, 1);
  		display_string("light value=");
   		display_int(sensorvalue, 1);						/* sensorvalue変数の中身を表示 */
		display_update();

//		while(1){								/* タッチセンサを押してスタート */
//			if( ecrobot_get_touch_sensor(TOUCH_SENSOR) == 1 ){
//				break;
//			}
//		}

		start_time = systick_get_ms();						/* 開始時刻を取得 */
		display_goto_xy(0,3);
		display_int(start_time,1);
		display_update();
		while( ecrobot_get_touch_sensor(TOUCH_SENSOR) == 0 )
		{

			current_time = systick_get_ms();				/* 現在時刻を取得 */
			display_goto_xy(0,4);
			display_int(current_time,1);
			display_update();
			if( current_time - start_time < 2000 ){				/* 動き始めてから2秒間は */
				off = 610;
			}
			else{
				off = GYRO_OFFSET;
			}

			light = ecrobot_get_light_sensor(LIGHT_SENSOR);
			before = standard;
			standard = light - gray;					/* 偏差を取得 */
			integral += (standard - before)/2.0 * DELTA_T;

			turn = p_gain * standard *100 / (black-white) + i_gain * integral / (black-white) * 100 + d_gain * (light-light_tmp) / (black-white) * 100;	/* 旋回値計算 */

			light_tmp = light;						/* 1つ前の明るさを格納 */

			/* 倒立振子制御(forward = 0, turn = 0で静止バランス) */
			balance_control(
				(float)FORWARD_POWER,					/* 前後進命令(+:前進, -:後進) */
				(float)turn,						/* 旋回命令(+:右旋回, -:左旋回) */
				(float)ecrobot_get_gyro_sensor(GYRO_SENSOR),		/* ジャイロセンサ値 */
//				(float)GYRO_OFFSET,					/* ジャイロセンサオフセット値 */
				(float)off,
				(float)nxt_motor_get_count(LEFT_MOTOR),			/* 左モータ回転角度[deg] */
				(float)nxt_motor_get_count(RIGHT_MOTOR),		/* 右モータ回転角度[deg] */
				(float)ecrobot_get_battery_voltage(),			/* バッテリ電圧[mV] */
				&pwm_L,							/* 左モータPWM出力値 */
				&pwm_R);						/* 右モータPWM出力値 */
			nxt_motor_set_speed(LEFT_MOTOR, pwm_L, 1);			/* 左モータPWM出力セット(-100〜100) */
			nxt_motor_set_speed(RIGHT_MOTOR, pwm_R, 1);			/* 右モータPWM出力セット(-100〜100) */

//			nxt_motor_set_speed( LEFT_MOTOR, forward-turn, 1 );
//			nxt_motor_set_speed( RIGHT_MOTOR, forward+turn, 1 );

//			ecrobot_bt_data_logger(100,-100);

			systick_wait_ms(4);

		}

	}

}
