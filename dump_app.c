void shoes_debug_upload(uint8_t* buf, uint16_t len)
{	
	uint16_t block_cnt = 0, remain_cnt = 0, i = 0;
	block_cnt = len / BLE_UART_MAX_CHAR_LEN;
	remain_cnt = len % BLE_UART_MAX_CHAR_LEN;

	for (i = 0; i < block_cnt; i++)
	{
		bt_gatt_notify(NULL,shoes_debug_value, buf + i * BLE_UART_MAX_CHAR_LEN,BLE_UART_MAX_CHAR_LEN,NULL);
		//local_task_sleep_ms(10);
	}

	if (remain_cnt > 0)
	{
		bt_gatt_notify(NULL,shoes_debug_value, buf + block_cnt*BLE_UART_MAX_CHAR_LEN, remain_cnt,NULL);  
	}	
}

ssize_t on_debug_data_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset)
{
	if (len!=2) return 0;

	if(!data_is_synchronizing){
		debug_data_write();
		data_is_synchronizing = true;
	}
			
	uint8_t para[2];
    	memcpy(para,buf,2);
	uint16_t  block_index =( (uint16_t)(para[1])        |
                         	 (uint16_t)(para[0]) << 8);  
    	
	uint8_t item_valid;
	uint16_t block_total;
	uint32_t item_id = spi_flash_get_debug_item_id();
	if( item_id%ITEM_NUM_ONE_BLOCK == 0)	 			
		block_total=(uint16_t)((item_id)/ITEM_NUM_ONE_BLOCK);
	else block_total=(uint16_t)((item_id)/ITEM_NUM_ONE_BLOCK+1);

	pr_info(LOG_MODULE_MAIN, "item_id:%d block_index:%d  block_total:%d", item_id,block_index, block_total);

	if(block_index >= block_total)  {
		shoes_debug_upload((uint8_t*)buf,len);
		data_is_synchronizing = false;		
		return 0;
	}
         
	if((item_id % ITEM_NUM_ONE_BLOCK) && (block_index == block_total-1))      			
		item_valid = (item_id) % ITEM_NUM_ONE_BLOCK;
	else
		item_valid = ITEM_NUM_ONE_BLOCK;		

	pr_info(LOG_MODULE_MAIN, "item_valid:%d", item_valid);

	debug_data_item_t debug_sport_data[ITEM_NUM_ONE_BLOCK];
	
	debug_data_read(block_index*ITEM_NUM_ONE_BLOCK,item_valid,debug_sport_data);
	for(int j=0;j<item_valid;j++)
		pr_info(LOG_MODULE_MAIN, "time:0x%x,battery_l:0x%x,battery_v:0x%x,boot_target:0x%x,wakelock:0x%x",debug_sport_data[j].time,debug_sport_data[j].battery_level,debug_sport_data[j].battery_vbat,debug_sport_data[j].boot_target,debug_sport_data[j].wakelock_lock);
	                		 
    uint8_t send_app_data_info[5+ITEM_NUM_ONE_BLOCK*13] = {
    	(uint8_t)(block_index >> 8),
    	(uint8_t)block_index,           
	
    	(uint8_t)(block_total >> 8),
    	(uint8_t)block_total,	

		item_valid*13,                   //len		
	};
			                  		       		
    uint16_t i = 0;
	for(i=0;i<item_valid;i++)
	{
		send_app_data_info[5+i*13]=(uint8_t)(debug_sport_data[i].time);
		send_app_data_info[6+i*13]=(uint8_t)(debug_sport_data[i].time >> 24);
		send_app_data_info[7+i*13]=(uint8_t)(debug_sport_data[i].time >> 16);
		send_app_data_info[8+i*13]=(uint8_t)(debug_sport_data[i].time >> 8);
		send_app_data_info[9+i*13]=(uint8_t)debug_sport_data[i].time;

		send_app_data_info[10+i*13]=(uint8_t)(debug_sport_data[i].battery_level >> 8);
		send_app_data_info[11+i*13]=(uint8_t)debug_sport_data[i].battery_level;

		send_app_data_info[12+i*13]=(uint8_t)(debug_sport_data[i].battery_vbat >> 8);
		send_app_data_info[13+i*13]=(uint8_t)debug_sport_data[i].battery_vbat;

		send_app_data_info[14+i*13]=(uint8_t)(debug_sport_data[i].boot_target >> 8);
		send_app_data_info[15+i*13]=(uint8_t)debug_sport_data[i].boot_target;

		send_app_data_info[16+i*13]=(uint8_t)(debug_sport_data[i].wakelock_lock >> 8);
		send_app_data_info[17+i*13]=(uint8_t)debug_sport_data[i].wakelock_lock;
	}

	if(block_index == block_total-1){
		debug_data_erase_item(item_id);
		data_is_synchronizing = false;
	}

	shoes_debug_upload(send_app_data_info,send_app_data_info[4]+5);
	
	return len;
}


