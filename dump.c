#include <stdlib.h>
#include <string.h>
#include "drivers/spi_flash.h"

#define  DEBUG_DATA_START         705
#define  DEBUG_DATA_NB_BLOCKS     255
#define  DEBUG_DATA_ITEM_COUNT    (255*255)

typedef struct {
  uint32_t  id;    
  uint32_t  time; 
  uint16_t  battery_level;
  uint16_t  battery_vbat;
  uint16_t  boot_target;
  uint16_t  wakelock_lock;
}debug_data_item_t;

static uint32_t spi_flash_get_debug_item_id(void)
{
	struct td_device *spi_flash_handler =(struct td_device *)&pf_sba_device_flash_spi0;

	unsigned int retlen;

	uint32_t item_cnt;

	uint32_t spi_addr = DEBUG_DATA_START * SERIAL_FLASH_BLOCK_SIZE;

	spi_flash_read(spi_flash_handler,spi_addr,1,&retlen,&item_cnt);

        if(item_cnt==0xffffffff) return 0;
          
        else return item_cnt;
}

static void spi_flash_write_debug_item(uint32_t itemOffset,uint8_t itemCount,void *data)
{
	struct td_device *spi_flash_handler =(struct td_device *)&pf_sba_device_flash_spi0;

	unsigned int retlen;
	
	DRIVER_API_RC ret = spi_flash_sector_erase(spi_flash_handler, DEBUG_DATA_START, 1);

	uint32_t spi_addr = DEBUG_DATA_START * SERIAL_FLASH_BLOCK_SIZE;

	uint32_t item_offset_v = itemOffset+itemCount;
	 
	ret = spi_flash_write(spi_flash_handler, spi_addr, 1,&retlen, &item_offset_v);
	 
	spi_addr += SERIAL_FLASH_BLOCK_SIZE * 1 + itemOffset*sizeof(debug_data_item_t);

	ret = spi_flash_write(spi_flash_handler, spi_addr , sizeof(debug_data_item_t)*itemCount/4,&retlen,data);

}

static void spi_flash_read_debug_item(uint32_t itemOffset,uint8_t itemCount, debug_data_item_t *data_read)
{
	struct td_device *spi_flash_handler =(struct td_device *)&pf_sba_device_flash_spi0;

	unsigned int retlen;
	
	uint32_t* data_tmp;

	data_tmp = balloc((itemCount) * sizeof(debug_data_item_t), NULL);

	uint32_t spi_addr = DEBUG_DATA_START * SERIAL_FLASH_BLOCK_SIZE;

	spi_addr += SERIAL_FLASH_BLOCK_SIZE * 1 + (itemOffset*sizeof(debug_data_item_t));
	spi_flash_read(spi_flash_handler, spi_addr ,sizeof(debug_data_item_t)*itemCount/4,&retlen,data_tmp);

	uint8_t i=0;
	uint8_t j=0;
	while(i < 4*itemCount)
	{
		data_read[j].id     =data_tmp[i]; 
		data_read[j].time   =data_tmp[i+1];
		data_read[j].battery_level  =(uint16_t)data_tmp[i+2];
		data_read[j].battery_vbat   =(uint16_t)(data_tmp[i+2]>>16);
		data_read[j].boot_target    =(uint16_t)data_tmp[i+3];
		data_read[j].wakelock_lock  =(uint16_t)(data_tmp[i+3]>>16);
		i=i+4;
		j++;
	}
	bfree(data_tmp);
}

extern uint16_t battery_level;
extern uint16_t battery_vbat;
extern enum get_boot_target();
extern uint16_t wakeup_lock();
void debug_data_write()
{
	debug_data_item_t data;
	uint32_t id_offset = spi_flash_get_debug_item_id();
	if(id_offset >= DEBUG_DATA_ITEM_COUNT) return;
	data.id=id_offset;
	data.time=utc_time_read();
	data.battery_level=battery_level;
	data.battery_vbat= battery_vbat;
	data.boot_target = (uint16_t)get_boot_target();
	data.wakelock_lock = wakeup_lock();
	spi_flash_write_debug_item(data.id,1,&data);
}

void debug_data_read(uint32_t itemOffset,uint8_t itemCount, debug_data_item_t *data_read)
{
	if(itemOffset >= DEBUG_DATA_ITEM_COUNT) return;
	spi_flash_read_debug_item(itemOffset,itemCount, data_read);
}

void debug_data_erase(void)
{
	struct td_device *spi_flash_handler =(struct td_device *)&pf_sba_device_flash_spi0;
	spi_flash_sector_erase(spi_flash_handler, DEBUG_DATA_START, DEBUG_DATA_NB_BLOCKS);
}

void debug_data_erase_item(uint32_t item_cnt)
{
	struct td_device *spi_flash_handler =(struct td_device *)&pf_sba_device_flash_spi0;
	spi_flash_sector_erase(spi_flash_handler,DEBUG_DATA_START, (item_cnt*sizeof(debug_data_item_t) / SERIAL_FLASH_BLOCK_SIZE)+2);
}

int main(void)
{
	time_60s_cnt++;

	if(time_60s_cnt % 12 == 0)	debug_data_write();

	return 0;
}

