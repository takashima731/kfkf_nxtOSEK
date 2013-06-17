/*
####################################################################################################
	name: kfkfModel.c
	Description: ???
	---
	update: 2013.06.13
####################################################################################################
*/

#include <stdlib.h>
#include "kfkfModel.h"

typedef struct tag_StateMachine {
	S16 num_of_events;
	S16 num_of_states;
	EvtType_e *events;
	State_t *states;
	S16 current_state;
} StateMachine_t;

/* NXT Bluetooth configuration */
#define BT_RCV_BUF_SIZE 32		/* Buffer size for bluetooth */

#define RESERVED_MATRIX_SIZE 3000
#define RESERVED_STATES_SIZE 900

S16 bt_receive_buf[BT_RCV_BUF_SIZE];	/* bluetooth */

static StateMachine_t statemachine;

/*
===============================================================================================
	name: receive_BT
	Description: ??
	Parameter: no
	Return Value: no
	---
	update: 2013.06.13
===============================================================================================
*/
void receive_BT(/* StateMachine_t statemachine*/){
	
	S16 matrix[RESERVED_MATRIX_SIZE];
	S16 states[RESERVED_STATES_SIZE];
	
    int packet_no = 1,	//packet number.
    int ptr = 0;

    display_clear(0);
    display_goto_xy(0, 1);
    display_string("BT Communication");
    display_update();
    
// packet type:1 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    while(1){
    	// Receive first packet.
        ecrobot_read_bt_packet(bt_receive_buf, BT_RCV_BUF_SIZE);
		
        if(bt_receive_buf[0] == packet_no && bt_receive_buf[1] == 1)
        {
        	statemachine.num_of_states = bt_receive_buf[2];
        	statemachine.num_of_events = bt_receive_buf[3];
			
            packet_no++;
            break;
        }
    }

// packet type:2 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    display_clear(0);
    display_goto_xy(0, 1);
    display_string("BT Communication");
    display_goto_xy(1, 1);
    display_string("receive packet:event");
    display_update();

    ptr = 0;

    while(1/*ptr+14 <num_of_states*num_of_events*/)
    {
        int i = 0;
        systick_wait_ms(100);
        
        ecrobot_read_bt_packet(bt_receive_buf, BT_RCV_BUF_SIZE);

        if(bt_receive_buf[1] == 3)
        {
            break;
        }

        if(bt_receive_buf[0] == packet_no && bt_receive_buf[1] == 2)
        {
            for(i=2;i<16;i++)
            {
                *(matrix+ptr) = *(bt_receive_buf+i);
                ptr++;
            }
            packet_no++;
        }
    }

    statemachine.events = (EvtType_e *)malloc(ptr);
    if(statemachine.events == NULL)
    {
        display_clear(0);
        display_goto_xy(0, 1);
        display_string("BT Communication");
        display_goto_xy(1, 1);
        display_string("Malloc Error:event");
        display_update();
    }

    for(i=0;i<ptr;i++)
    {
    	statemachine.events[i] = (EvtType_e)matrix[i];
    }
    

// packet type:3 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    display_clear(0);
    display_goto_xy(0, 1);
    display_string("BT Communication");
    display_goto_xy(1, 1);
    display_string("end packet:event");
    display_goto_xy(2, 1);
    display_string("receive packet:state");
    display_update();

    ptr = 0;

    statemachine.states = (State_t *)malloc(statemachine.num_of_states);
    if(statemachine.states == NULL)
    {
        display_clear(0);
        display_goto_xy(0, 1);
        display_string("BT Communication");
        display_goto_xy(1, 1);
        display_string("end packet:event");
        display_goto_xy(2, 1);
        display_string("Malloc Error:state");
        display_update();
    }

    while(1/*ptr+14 <(2+4)*num_of_states*/){

        int i=0;
        systick_wait_ms(100);

        if(ptr!=0)
        {
            ecrobot_read_bt_packet(bt_receive_buf, BT_RCV_BUF_SIZE);
        }
        if(bt_receive_buf[1] == 255)
        {
            break;
        }
        
        if(bt_receive_buf[0] == packet_no && bt_receive_buf[1] == 3)
        {
            for(i=2;i<16;i++)
            {
                *(states+ptr) = *(bt_receive_buf+i);
                ptr++;
            }
            packet_no++;
        }
    }
    
    for(i=0;i<ptr;i=i+6)
    {
    	statemachine[i].states.state_no = states[i];
    	statemachine[i+1].states.action_no = (ActType_e)states[i+1];
    	statemachine[i+2].states.value0 = states[i+2];
    	statemachine[i+3].states.value1 = states[i+3];
    	statemachine[i+4].states.value2 = states[i+4];
    	statemachine[i+5].states.value3 = states[i+5];
    }

    display_clear(0);
    display_goto_xy(0, 1);
    display_string("BT Communication");
    display_goto_xy(1, 1);
    display_string("end packet:event");
    display_goto_xy(2, 1);
    display_string("end packet:state");
    display_update();

    statemachine.current_state = 0;

}

/*
===============================================================================================
	name: get_CurrentState
	Description: ??
	Parameter: no
	Return Value: statemachine.current_state
	---
	update: 2013.06.13
===============================================================================================
*/
S16 getCurrentState()
{
	return statemachine.current_state;
}

/*
===============================================================================================
	name: set_NextState(befote:sendevent)
	Description: ??
	Parameter: event_id:S8
	Return Value:
	---
	update: 2013.06.13
===============================================================================================
*/
State_t setNextState(EvtType_e event_id) {
	S16 next_state = -1;
	S16 i = 0;
	State_t nostate = {-1,NO_INPUT,0,0,0,0};

	next_state = statemachine.events[event_id + statemachine.current_state * statemachine.num_of_events];

	if(next_state == -1) return nostate;

	statemachine.current_state = next_state;
/*
	for(i = 0;i < statemachine.num_of_states;i++) {
		if(i == statemachine.current_state) {
			ControllerSet(&statemachine.states[i]);
			return 1;
		}
	}
*/
	return statemachine.states[statemachine.current_state];
}

/*
===============================================================================================
	name: BluetoothStart
	Description: ??
	Parameter: no
	Return Value: S8
	---
	update: 2013.06.17
===============================================================================================
*/
S8 BluetoothStart()
{
	boolean btstart = OFF;

	ecrobot_read_bt_packet(bt_receive_buf, BT_RCV_BUF_SIZE);
	if(bt_receive_buf[1] == 254 )
	{
		btstart = ON;
	}
	else
	{
		btstart = OFF;
	}

	return btstart;
}