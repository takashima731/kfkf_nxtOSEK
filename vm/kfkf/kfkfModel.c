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

/*
===============================================================================================
	Definition
===============================================================================================
*/
/* Buffer size for Bluetooth */
#define BT_RCV_BUF_SIZE 32
/* The number of event */
#define RESERVED_EVENT_SIZE 3000
/* The number of state */
#define RESERVED_STATES_SIZE 900

/* Definition of Structure of state machine */
typedef struct tag_StateMachine {
	S16 num_of_events;
	S16 num_of_states;
	EvtType_e *events;
	State_t *states;
	S16 current_state;
} StateMachine_t;

/*
===============================================================================================
	Variables
===============================================================================================
*/
/* Buffer for Bluetooth */
U8 bt_receive_buf[BT_RCV_BUF_SIZE];
/* State Machine for kfkf Model */
static StateMachine_t g_StateMachine;


/*
===============================================================================================
	Functions
===============================================================================================
*/
/*
===============================================================================================
	name: receive_BT
	Description: ??
	Parameter: no
	Return Value: no
===============================================================================================
*/
static U16 g_PacketCnt;
static U16 g_Eptr;
static U16 g_Sptr;

static S16 events[RESERVED_EVENT_SIZE];
static S16 states[RESERVED_STATES_SIZE];

U8 ReceiveBT(void){
	
    U16 i = 0;
    U16 j = 0;
    U8 comm_end = OFF;

    ecrobot_read_bt_packet(bt_receive_buf, BT_RCV_BUF_SIZE);

//-------------------------------------------------------------------------------------
//  If packet end
//-------------------------------------------------------------------------------------
    if(bt_receive_buf[1] == 255)
    {
		//==========================================
		//	Allocation for events
		//==========================================
        g_StateMachine.events = (EvtType_e *)malloc(g_Eptr);
        if(g_StateMachine.events == NULL)
        {
            display_clear(0);
            display_goto_xy(0, 1);
            display_string("Pre:Bluetooth");
            display_goto_xy(0, 2);
            display_string("Malloc Err:event");
            display_update();
            ecrobot_sound_tone(880, 50, 30);
        }
        else
        {
        	for(i=0;i<g_Eptr;i++)
        	{
        		g_StateMachine.events[i] = (EvtType_e)events[i];
        	}

        	comm_end++;
        }


		//==========================================
		//	Allocation for states
		//==========================================
        g_StateMachine.states = (State_t *)malloc(g_StateMachine.num_of_states);
        if(g_StateMachine.states == NULL)
        {
            display_clear(0);
            display_goto_xy(0, 1);
            display_string("Pre:Bluetooth");
            display_goto_xy(0, 2);
            display_string("Malloc Err:state");
            display_update();
            ecrobot_sound_tone(880, 50, 30);
        }
        else
        {
        	j = 0;
        	for(i=0;i<g_Sptr;i=i+6)
        	{
        		g_StateMachine.states[j].state_no = states[i];
        		g_StateMachine.states[j].action_no = (ActType_e)states[i+1];
       			g_StateMachine.states[j].value0 = states[i+2];
       			g_StateMachine.states[j].value1 = states[i+3];
       			g_StateMachine.states[j].value2 = states[i+4];
       			g_StateMachine.states[j].value3 = states[i+5];
       			j++;
       		}

        	comm_end++;
        }

        g_StateMachine.current_state = 0;

        if(comm_end >= 2)
        {
        	comm_end = ON;
        }
    }

//-------------------------------------------------------------------------------------
//  If packet type:1
//-------------------------------------------------------------------------------------
    if(bt_receive_buf[0] == g_PacketCnt && bt_receive_buf[1] == 1)
    {
    	g_StateMachine.num_of_states = bt_receive_buf[2];
    	g_StateMachine.num_of_events = bt_receive_buf[3];
		
    	g_PacketCnt++;
    }

//-------------------------------------------------------------------------------------
//  If packet type:2
//-------------------------------------------------------------------------------------
    if(bt_receive_buf[0] == g_PacketCnt && bt_receive_buf[1] == 2)
    {
    	for(i=2;i<16;i++)
    	{
    		*(events + g_Eptr) = *(bt_receive_buf + i);
    		g_Eptr++;
    	}
    	g_PacketCnt++;
    }

//-------------------------------------------------------------------------------------
//  If packet type:3
//-------------------------------------------------------------------------------------
    if(bt_receive_buf[0] == g_PacketCnt && bt_receive_buf[1] == 3)
    {
    	for(i=2;i<16;i++)
    	{
    		*(states + g_Sptr) = *(bt_receive_buf + i);
    		g_Sptr++;
    	}

    	g_PacketCnt++;
    }
    

	//==========================================
	//	End of BT communication
    // -------
    // Not END:OFF / END:ON
	//==========================================
    return comm_end;
}


/*
===============================================================================================
	name: get_CurrentState
	Description: ??
	Parameter: no
	Return Value: g_StateMachine.current_state
===============================================================================================
*/
S16 getCurrentState()
{
	return g_StateMachine.current_state;
}

/*
===============================================================================================
	name: set_NextState(befote:sendevent)
	Description: ??
	Parameter: event_id:S8
	Return Value:
===============================================================================================
*/
#define NO_STATE -1

State_t setNextState(EvtType_e event_id) {
	S8 next_state = NO_STATE;
	//S16 i = 0;
	State_t state = {-1,-1,0,0,0,0};

	if(g_StateMachine.events != NULL)
	{
		next_state = g_StateMachine.events[event_id + g_StateMachine.current_state * g_StateMachine.num_of_events];
	}

	if(next_state != NO_STATE)
	{
		g_StateMachine.current_state = next_state;
		state = g_StateMachine.states[g_StateMachine.current_state];
	}

/*
	for(i = 0;i < g_StateMachine.num_of_states;i++) {
		if(i == g_StateMachine.current_state) {
			ControllerSet(&g_StateMachine.states[i]);
			return 1;
		}
	}
*/
	return state;
}

/*
===============================================================================================
	name: BluetoothStart
	Description: ??
	Parameter: no
	Return Value: S8
===============================================================================================
*/
S8 BluetoothStart(void)
{
	U8 btstart = OFF;

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


/*
===============================================================================================
	name: InitKFKF
	Description: ??
	Parameter: no
	Return Value: no
===============================================================================================
*/
void InitKFKF(void)
{
	U16 i = 0;

	//==========================================
	//	Initialization of StateMachine
	//==========================================
	g_StateMachine.num_of_events = 0;
	g_StateMachine.num_of_states = 0;
	g_StateMachine.current_state = 0;

	free( g_StateMachine.events );
	g_StateMachine.events = NULL;

	free( g_StateMachine.states );
	g_StateMachine.states = NULL;


	//==========================================
	//	Initialization of others
	//==========================================
	g_PacketCnt = 1;
	g_Eptr = 0;
	g_Sptr = 0;

	for(i=0;i<RESERVED_EVENT_SIZE;i++)
	{
		events[i] = 0;
	}
	for(i=0;i<RESERVED_STATES_SIZE;i++)
	{
		states[i] = 0;
	}
}
