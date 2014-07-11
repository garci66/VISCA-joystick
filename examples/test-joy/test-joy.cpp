// test-joy.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"




/*The device the camera is attached to*/
/*The default device the camera is attached to*/
char *ttydev = "COM3:";

/*Structures needed for the VISCA library*/

int quit=0;  //Global quit variable - accessible from other threads


int _tmain(int argc, _TCHAR* argv[])
{
	VISCAInterface_t iface;
	VISCACamera_t cameras[MAX_CAMS];
	//VISCACamera_t camera;
	int current_camera=0;
	int max_cameras=0;
	
	JOYINFOEX joyInfoEx;
	JOYCAPS joyCaps;
	int JoyPresent;
	uint8_t max_pan_speed, max_tilt_speed;
	double pan_speed_scale, tilt_speed_scale, zoom_speed_scale;
	SAVED_POINT positions[MAX_MEMS];
	SAVED_POINT temp_point ={0,0,0,0,0,0};
	for (int j=0;j<MAX_MEMS;j++){
		positions[j]=temp_point;
	}
	
	if( SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ) ) 
	{ 
		_RPTF0(_CRT_WARN, "\nThe Control Handler is installed.\n" );
		
	} 
	else 
	{
		printf( "\nERROR: Could not set control handler"); 
		return 1;
	}


	    // Register the window class.
    
	open_interface(&iface);

	printf("Total cameras detected: %d\n",iface.camera_num);
	max_cameras=iface.camera_num;
		
	for (int i=0;i<max_cameras;i++)
	{
		cameras[i].address=i+1;
		VISCA_get_camera_info(&iface, &cameras[i]);
		printf("Camera: %d - vendor: 0x%04x\n model: 0x%04x\n ROM version: 0x%04x\n socket number: 0x%02x\n",
			cameras[i].address,	cameras[i].vendor, cameras[i].model, cameras[i].rom_version, cameras[i].socket_num);
		VISCA_set_dzoom(&iface, &cameras[i], 3); //disable digital zoom - value is 3 to disable!
	}
	//VISCA_get_camera_info(&iface, &camera);
	//fprintf(stderr,"Some camera info:\n------------------\n");
	//fprintf(stderr,"vendor: 0x%04x\n model: 0x%04x\n ROM version: 0x%04x\n socket number: 0x%02x\n",
	//	camera.vendor, camera.model, camera.rom_version, camera.socket_num);

	//define max tilt speeds based on camera features
	
	VISCA_get_pantilt_maxspeed(&iface, &cameras[current_camera], &max_pan_speed, &max_tilt_speed);
	_RPTF2(_CRT_WARN, "max_speeds: pan:%d, tilt:%d\n",max_pan_speed,max_tilt_speed);
	
	pan_speed_scale=((double)max_pan_speed)/32768;
	tilt_speed_scale=((double)max_tilt_speed)/32768;
	zoom_speed_scale=((double)7/32768);


	joyInfoEx.dwSize = sizeof(joyInfoEx);
	joyGetDevCaps(JOYSTICKID1, &joyCaps, sizeof(joyCaps));
	JoyPresent = (joyGetPosEx(JOYSTICKID1, &joyInfoEx) == JOYERR_NOERROR);

	int pan_dir=PANSTOP, tilt_dir=TILTSTOP, zoom_dir=ZOOMSTOP;
	int old_pan_dir=PANSTOP, old_tilt_dir=TILTSTOP, old_zoom_dir=ZOOMSTOP;

	int pan_speed=0, tilt_speed=0, zoom_speed=0;
	int delta_pan, delta_tilt, delta_zoom;
	DWORD past_buttons=0;
	DWORD edge_buttons=0;
	bool backlight[MAX_CAMS]={false};
	//bool backlight=false;


	while(JoyPresent&&(!quit))
	{
		Sleep(50);

		JoyPresent = (joyGetPosEx(JOYSTICKID1, &joyInfoEx) == JOYERR_NOERROR);

		if (JoyPresent)
		{
			joyInfoEx.dwSize = sizeof(joyInfoEx);
			joyInfoEx.dwFlags = JOY_RETURNALL;
			joyGetPosEx(JOYSTICKID1, &joyInfoEx);
		} else {
			// If joystick not detected, quit.
			quit =1;
		}

		//Compare old buttons (past poll) and new - report only the deltas on those pressed
		edge_buttons=(past_buttons^joyInfoEx.dwButtons)&joyInfoEx.dwButtons;
		//Store the current button presses as the "old" for next iteration
		past_buttons=joyInfoEx.dwButtons;

		
		//Use button 6 to control backlight on/off
		
		if (edge_buttons & BUTTON6){
			if (!backlight[current_camera]){
				VISCA_set_backlight_comp(&iface, &cameras[current_camera],2);
				printf("Camera %d - Backlight on\n",current_camera+1);
				backlight[current_camera]=true;
			}else{
				VISCA_set_backlight_comp(&iface, &cameras[current_camera],3);
				printf("Camera %d - Backlight off\n",current_camera+1);
				backlight[current_camera]=false;
			}
		}

		//Use button 3 to decrement current camera - button 4 to increment
		if (edge_buttons & (BUTTON3|BUTTON4))
		{
			int old_camera=current_camera;
			switch (edge_buttons & (BUTTON3|BUTTON4))
			{
			case BUTTON3:
				current_camera=max(current_camera-1,0);
				break;
			case BUTTON4:
				current_camera=min(current_camera+1,max_cameras-1);
				break;
			}
			if (current_camera != old_camera)
			{
				printf("Current camera is now: %d\n",current_camera+1);
				stopcamera(&iface, &cameras[old_camera]);
			}
		}


		if (joyInfoEx.dwButtons	& BUTTON2){
			// Use actual button press as we are using Button 2 as modifier
			//Enter program mode to save position - use button 2+memory number - 7..12 to save
			temp_point.pan_pos=0;
			temp_point.tilt_pos=0;
			temp_point.zoom_pos=0;
			temp_point.saved=false;
			temp_point.backlight=false;

			if ( edge_buttons & (BUTTON7|BUTTON8|BUTTON9|BUTTON10|BUTTON11|BUTTON12)){
				
				int memory_pos=0;
				//Get current camera position
				VISCA_get_pantilt_position(&iface, &cameras[current_camera], &temp_point.pan_pos, &temp_point.tilt_pos);
				VISCA_get_zoom_value(&iface, &cameras[current_camera], &temp_point.zoom_pos);
				temp_point.camera=current_camera;
				temp_point.saved=true;
				temp_point.backlight=backlight[current_camera];

				switch (edge_buttons & (BUTTON7|BUTTON8|BUTTON9|BUTTON10|BUTTON11|BUTTON12))
				{
				case BUTTON7:
					memory_pos=0;
					break;
				case BUTTON8:
					memory_pos=1;
					break;
				case BUTTON9:
					memory_pos=2;
					break;
				case BUTTON10:
					memory_pos=3;
					break;
				case BUTTON11:
					memory_pos=4;
					break;
				case BUTTON12:
					memory_pos=5;
					break;
				case BUTTON13:
					memory_pos=6;
					break;
				case BUTTON14:
					memory_pos=7;
					break;
				case BUTTON15:
					memory_pos=8;
					break;
				case BUTTON16:
					memory_pos=9;
					break;
				}
				positions[memory_pos]=temp_point;
				printf("Saving position to memory %d - Pan:%d Tilt:%d Zoom:%d Backlight:%s\n",
					memory_pos+1, temp_point.pan_pos,temp_point.tilt_pos,temp_point.zoom_pos,temp_point.backlight?"on":"off");
			}
			
		}else if ( edge_buttons & (BUTTON7|BUTTON8|BUTTON9|BUTTON10|BUTTON11|BUTTON12)){
			// recall button pressed - lets recall a memory
			int memory_pos=0;
			switch (edge_buttons)
			{
			case BUTTON7:
				memory_pos=0;
				break;
			case BUTTON8:
				memory_pos=1;
				break;
			case BUTTON9:
				memory_pos=2;
				break;
			case BUTTON10:
				memory_pos=3;
				break;
			case BUTTON11:
				memory_pos=4;
				break;
			case BUTTON12:
				memory_pos=5;
				break;
			case BUTTON13:
				memory_pos=6;
				break;
			case BUTTON14:
				memory_pos=7;
				break;
			case BUTTON15:
				memory_pos=8;
				break;
			case BUTTON16:
				memory_pos=9;
				break;
			}
			temp_point=positions[memory_pos];
			if (temp_point.saved==true && temp_point.camera<max_cameras){
				int restore_camera=temp_point.camera;
				printf("Restoring position # %d for Camera %d - input stopped till completed\n",memory_pos+1, restore_camera+1);
				printf("Restore point - Pan:%d Tilt:%d Zoom:%d Backlight:%s\n",
					temp_point.pan_pos,temp_point.tilt_pos,temp_point.zoom_pos, temp_point.backlight?"on":"off");
				VISCA_set_pantilt_absolute_position(&iface, &cameras[restore_camera], max_pan_speed,max_tilt_speed,temp_point.pan_pos, temp_point.tilt_pos);
				VISCA_set_zoom_value(&iface, &cameras[restore_camera],temp_point.zoom_pos);
				VISCA_set_backlight_comp(&iface, &cameras[restore_camera],temp_point.backlight?2:3);
				printf("Position restored - Input resumed\n");
			} else {
				printf("No position saved in this memory slot\n");
			}
		}

		if (joyInfoEx.dwButtons & BUTTON1)
		{
			//system("cls");
			for (int i=0;i<max_cameras;i++)
			{
				VISCA_get_pantilt_position(&iface, &cameras[i], &temp_point.pan_pos, &temp_point.tilt_pos);
				VISCA_get_zoom_value(&iface, &cameras[i], &temp_point.zoom_pos);
				temp_point.backlight=backlight[i];
				temp_point.saved=false;
				printf("Camera %d position - Pan:%5d Tilt:%5d Zoom:%5d Backlight:%s\n",
					i+1, temp_point.pan_pos,temp_point.tilt_pos,temp_point.zoom_pos,temp_point.backlight?"on":"off");
			}

		}

	/*	if ( joyInfoEx.dwPOV == POV_N )	{
			printf("up\n");
		}
		if ( joyInfoEx.dwPOV & 0x1 ){
			printf("no position\n");
		}
		*/

		//_RPTF4(_CRT_WARN,"X:%d Y:%d Z:%d T:%d\n", joyInfoEx.dwXpos, joyInfoEx.dwYpos, joyInfoEx.dwZpos, joyInfoEx.dwRpos); 

		delta_pan = (int)joyInfoEx.dwXpos - JOY_MIDP;
		delta_tilt = (int)joyInfoEx.dwYpos - JOY_MIDP;
		delta_zoom = (int)joyInfoEx.dwRpos - JOY_MIDP;

		if ( abs(delta_pan) > JOY_DEADZONE ){
			pan_dir = (delta_pan>0)?PANRIGHT:PANLEFT;
			pan_speed = (int)floor((abs(delta_pan)-JOY_DEADZONE)*pan_speed_scale);
			pan_dir=(pan_speed>0)?pan_dir:PANSTOP; //if effective camera speed is 0, then stop
		}else{
			pan_dir=PANSTOP;
			pan_speed = 0;
		}

		if ( abs(delta_tilt) > JOY_DEADZONE ){
			tilt_dir = (delta_tilt>0)?TILTUP:TILTDOWN;
			tilt_speed = (int)floor((abs(delta_tilt)-JOY_DEADZONE)*tilt_speed_scale);
			tilt_dir=(tilt_speed>0)?tilt_dir:TILTSTOP; //if effective camera speed is 0, then stop
		}else{
			tilt_dir=TILTSTOP;
			tilt_speed = 0;
		}

		if ( abs(delta_zoom) > JOY_DEADZONE ){
			zoom_dir = (delta_zoom>0)?ZOOMTELE:ZOOMWIDE;
			zoom_speed = (int)floor((abs(delta_zoom)-JOY_DEADZONE)*zoom_speed_scale);
			zoom_dir=(zoom_speed>0)?zoom_dir:ZOOMSTOP;
			if (zoom_dir==ZOOMTELE){
				VISCA_set_zoom_tele_speed(&iface, &cameras[current_camera], zoom_speed);
			}else if (zoom_dir==ZOOMWIDE){
				VISCA_set_zoom_wide_speed(&iface, &cameras[current_camera], zoom_speed);
			}else{
				VISCA_set_zoom_stop(&iface, &cameras[current_camera]);
			}
			old_zoom_dir=zoom_dir;
		}else{
			zoom_dir=ZOOMSTOP;
			zoom_speed = 0;
			if (old_zoom_dir != ZOOMSTOP){
				VISCA_set_zoom_stop(&iface, &cameras[current_camera]);
			}
			old_zoom_dir=zoom_dir;
		}

		if (!( (pan_dir == PANSTOP)&&(tilt_dir==TILTSTOP)&&((pan_dir==old_pan_dir)&&(tilt_dir==old_tilt_dir)))){
			//position has changed and was NOT stop!
			pantilt(pan_dir,pan_speed,tilt_dir,tilt_speed, &iface, &cameras[current_camera]);
		}
		old_pan_dir=pan_dir;
		old_tilt_dir=tilt_dir;
		
	}

	printf("Program closing\n");
	close_interface(&iface);

}


void stopcamera(VISCAInterface_t *iface, VISCACamera_t *camera )
{
	VISCA_set_zoom_stop(iface, camera);
	VISCA_set_pantilt_stop(iface, camera, 0, 0);
	return;
}

void pantilt( int pandir, int panspeed, int tiltdir, int tiltspeed,VISCAInterface_t *iface, VISCACamera_t *camera  ){
	_RPTF4(_CRT_WARN, "in pantilt... %d %d %d %d\n", pandir, panspeed, tiltdir, tiltspeed );
	if ( pandir == PANSTOP && tiltdir == TILTSTOP )
	{
		VISCA_set_pantilt_stop(iface, camera, panspeed, tiltspeed);
	}
	else if ( pandir == PANSTOP && tiltdir == TILTUP )
	{
		VISCA_set_pantilt_up(iface, camera, panspeed, tiltspeed);
	}
	else if ( pandir == PANSTOP && tiltdir == TILTDOWN )
	{
		VISCA_set_pantilt_down(iface, camera, panspeed, tiltspeed);
	}
	else if ( pandir == PANRIGHT && tiltdir == TILTSTOP )
	{
		VISCA_set_pantilt_right(iface, camera, panspeed, tiltspeed);
	}
	else if ( pandir == PANLEFT && tiltdir == TILTSTOP )
	{
		VISCA_set_pantilt_left(iface, camera, panspeed, tiltspeed);
	}
	else if ( pandir == PANRIGHT && tiltdir == TILTUP )
	{
		VISCA_set_pantilt_upright(iface, camera, panspeed, tiltspeed);
	}
	else if ( pandir == PANRIGHT && tiltdir == TILTDOWN )
	{
		VISCA_set_pantilt_downright(iface, camera, panspeed, tiltspeed);
	}
	else if ( pandir == PANLEFT && tiltdir == TILTUP )
	{
		VISCA_set_pantilt_upleft(iface, camera, panspeed, tiltspeed);
	}
	else if ( pandir == PANLEFT && tiltdir == TILTDOWN )
	{
		VISCA_set_pantilt_downleft(iface, camera, panspeed, tiltspeed);
	}

	return;
}




BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ 
	switch( fdwCtrlType ) 
	{ 
		// Handle the CTRL-C signal. 
	case CTRL_C_EVENT: 
		printf( "Ctrl-C event\n\n" );
		Beep( 750, 300 ); 
		quit=1;
		return( TRUE );

		// CTRL-CLOSE: confirm that the user wants to exit. 
	case CTRL_CLOSE_EVENT: 
		Beep( 600, 200 ); 
		printf( "Ctrl-Close event\n\n" );
		quit=1;
		return( TRUE ); 

		// Pass other signals to the next handler. 
	case CTRL_BREAK_EVENT: 
		Beep( 900, 200 ); 
		printf( "Ctrl-Break event\n\n" );
		quit=1;
		return TRUE; 

	case CTRL_LOGOFF_EVENT: 
		Beep( 1000, 200 ); 
		printf( "Ctrl-Logoff event\n\n" );
		quit=1;
		return TRUE; 

	case CTRL_SHUTDOWN_EVENT: 
		Beep( 750, 500 ); 
		printf( "Ctrl-Shutdown event\n\n" );
		quit=1;
		return TRUE; 

	default: 
		return FALSE; 
	} 
}


void print_usage() {
	fprintf(stderr,"Usage: visca-cli [-d <serial port device>] \n");
	fprintf(stderr,"  default serial port device: %s\n",ttydev);      
	exit(1);  
}

void open_interface(VISCAInterface_t *pIface) {
	
	VISCACamera_t camera;	//Dummy camera used for init purposes
	if (VISCA_open_serial(pIface, ttydev)!=VISCA_SUCCESS) {
		fprintf(stderr,"visca-cli: unable to open serial device %s\n",ttydev);
		exit(1);
	}

	pIface->broadcast=0;
	VISCA_set_address(pIface);
	if(VISCA_set_address(pIface)!=VISCA_SUCCESS) {
#ifdef WIN
		_RPTF0(_CRT_WARN,"unable to set address\n");
#endif
		fprintf(stderr,"visca-cli: unable to set address\n");
		VISCA_close_serial(pIface);
		exit(1);
	}

	camera.address=1;  //We should have at least one camera


	if(VISCA_clear(pIface, &camera)!=VISCA_SUCCESS) {
#ifdef WIN
		_RPTF0(_CRT_WARN,"unable to clear interface\n");
#endif
		fprintf(stderr,"visca-cli: unable to clear interface\n");
		VISCA_close_serial(pIface);
		exit(1);
	}
	if(VISCA_get_camera_info(pIface, &camera)!=VISCA_SUCCESS) {
#ifdef WIN
		_RPTF0(_CRT_WARN,"unable to oget camera infos\n");
#endif
		fprintf(stderr,"visca-cli: unable to oget camera infos\n");
		VISCA_close_serial(pIface);
		exit(1);
	}

#if DEBUG 
	fprintf(stderr,"Camera initialisation successful.\n");
#endif
}

void close_interface(VISCAInterface_t *pIface)
{
	// read the rest of the data: (should be empty)
	unsigned char packet[3000];
	uint32_t buffer_size = 3000;

	VISCA_usleep(2000);

	if (VISCA_unread_bytes(pIface, packet, &buffer_size)!=VISCA_SUCCESS)
	{
		uint32_t i;
		fprintf(stderr, "ERROR: %u bytes not processed", buffer_size);
		for (i=0;i<buffer_size;i++)
			fprintf(stderr,"%2x ",packet[i]);
		fprintf(stderr,"\n");
	}

	VISCA_close_serial(pIface);
}

int process_commandline(int argc, char **argv) {


	/*at least a command has to be specified*/
	if (argc >0 && argc !=2 ) {
		print_usage();
		return 1;
	}

	/*Find the ttydev if specified*/
	if (argc == 2){
		if (strncmp(argv[1], "-d", 2) == 0) {
			/*after the -d and the device name at least one command has to follow*/
			print_usage();
			return 1;
		} else {
			ttydev = argv[2];
		}
	}
	return 0;
}
