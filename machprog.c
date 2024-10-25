#include <unistd.h>				//Needed for I2C port
#include <fcntl.h>				//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c.h>
#include <linux/i2c-dev.h>		//Needed for I2C port
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#define ERASE_SRAM (1 << 16)
#define ERASE_FEATURE_ROW (1 << 17)
#define ERASE_CONGFIG_FLASH (1 << 18)
#define ERASE_UFM (1 << 19)

#define I2C_ADDR 0x40

int file_i2c;

int i2c_write(uint8_t *buf, uint16_t len)
{
	int ret;
	
	ret = write(file_i2c, buf, len);
	if (ret != len)
	{
		printf("i2c_write expected to write %d but returned %d\n",len,ret);
	}
	return ret;
}

int i2c_read(uint8_t *buf, uint16_t len)
{
	int ret;
	
	ret = read(file_i2c, buf, len);
	if (ret != len)
	{
		printf("i2c_read expected to read %d but returned %d\n",len,ret);
	}
	return ret;	
}

int i2c_rdwr(uint8_t *bufout, uint16_t outlen, uint8_t *bufin, uint16_t lenin)
{
    int retval;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msgset[1];

    msgs[0].addr = I2C_ADDR;
    msgs[0].flags = 0;
    msgs[0].len = outlen;
    msgs[0].buf = bufout;

    msgs[1].addr = I2C_ADDR;
    msgs[1].flags = I2C_M_RD | I2C_M_NOSTART;
    msgs[1].len = lenin;
    msgs[1].buf = bufin;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 2;

    if (ioctl(file_i2c, I2C_RDWR, &msgset) < 0) {
        perror("ioctl(I2C_RDWR) in i2c_read");
        return -1;
    }

    return 0;
}

uint32_t read_generic_32bit(uint8_t val)
{
	uint32_t rval;
	int ret;
	uint8_t buf[4];
	uint8_t bufin[4];

	memset(buf,0,4);
	buf[0] = val;
	ret = i2c_rdwr(buf,4,bufin,4);
	
	rval = bufin[0];
	rval <<= 8;
	rval |= bufin[1];
	rval <<= 8;
	rval |= bufin[2];
	rval <<= 8;
	rval |= bufin[3];
    return rval;
}

int read_into_byte_buffer(uint8_t cmd, uint8_t *bufin, uint16_t len)
{
	int ret;

	uint8_t buf[4];

	memset(buf,0,4);
	buf[0] = cmd;
	ret = i2c_rdwr(buf,4,bufin,len);
//	ret=i2c_write(buf,4);
//	ret=i2c_read(bufin,len);
	return ret;
}

void read_into_byte_buffer2(uint8_t cmd, uint8_t cmd2, uint8_t *bufin, uint16_t len)
{
	int ret;

	uint8_t buf[4];

	memset(buf,0,4);
	buf[0] = cmd;
	buf[3] = cmd2;
	ret = i2c_rdwr(buf,4,bufin,len);
}

uint32_t read_device_id(void)
{
	return read_generic_32bit(0xe0);
}

uint32_t read_user_code(void)
{
	return read_generic_32bit(0xc0);
}

uint32_t read_status(void)
{
	return read_generic_32bit(0x3c);
}


void read_feature_row(uint8_t *buf)
{
	read_into_byte_buffer(0xe7,buf,8);
}

uint32_t read_feature_bits(uint8_t *buf)
{
	read_into_byte_buffer(0xfb, buf, 2);
}

void read_flash(uint8_t *buf)
{
	read_into_byte_buffer2(0x73,0x01, buf, 16);
}

void read_ufm(uint8_t *buf)
{
	read_into_byte_buffer2(0xca,0x01,buf,16);
}

void read_otp_fuses(uint8_t *buf)
{
	read_into_byte_buffer(0xfa, buf, 1);
}

void erase_ufm(void)
{
	uint8_t buf[4];
	memset(buf,0,4);
	buf[0] = 0xcb;
	i2c_write(buf,4);
}

void erase(uint32_t flags)
{
	uint8_t buf[4];
	buf[0] = 0x0e;
	buf[1] = flags >> 16;
	buf[2] = flags >> 8;
	buf[3] = flags & 0xff;
	i2c_write(buf,4);
}

void enable_config_transparent(void)
{
	uint8_t buf[3];
	memset(buf,0,3);
	buf[0] = 0x74;
	buf[1] = 0x08;
	i2c_write(buf,3);
}

void enable_config_offline(void)
{
	uint8_t buf[3];
	memset(buf,0,3);
	buf[0] = 0xc6;
	buf[1] = 0x08;
	i2c_write(buf,3);

}

int check_busy(void)
{
	int ret;
	uint8_t buf[4];
	uint8_t bufin[1];
	memset(buf,0,4);
	buf[0] = 0xf0;
	i2c_rdwr(buf,4,bufin,1);
	if (bufin[0] & 0x80)
		ret = -1;
	else
		ret = 0;
	return ret;
}

void wait_busy(void)
{
	while (check_busy() != 0)
	{
		usleep(10000);
	}
}

void reset_config_addr(void)
{
	uint8_t buf[4];
	memset(buf,0,4);
	buf[0] = 0x46;
	i2c_write(buf,4);
}

void set_config_addr(uint16_t page)
{
	uint8_t buf[8];
	memset(buf,0,8);
	buf[0] = 0x46;
	buf[6] =  page >> 8;
	buf[7] = page & 0xff;
	i2c_write(buf,8);
}

void set_ufm_addr(uint16_t page)
{
	uint8_t buf[8];
	memset(buf,0,8);
	buf[0] = 0xb4;
	buf[4] = 0x40;
	buf[6] =  page >> 8;
	buf[7] = page & 0xff;
	i2c_write(buf,8);
}

void program_page(uint8_t *pdata)
{
	uint8_t buf[20];
	memset(buf,0,20);
	buf[0] = 0x70;
	buf[4] = 0x01;
	memcpy(&buf[4],pdata,16);
	i2c_write(buf,20);
}

void program_done(void)
{
	uint8_t buf[4];
	memset(buf,0,4);
	buf[0] = 0x5e;
	i2c_write(buf,4);	
}

void refresh(void)
{
	uint8_t buf[3];
	memset(buf,0,3);
	buf[0] = 0x79;
	i2c_write(buf,3);	
}

void wakeup(void)
{
	uint8_t buf[4];
	memset(buf,0xff,4);
	i2c_write(buf,4);
}
uint16_t cvt_to_array(char *linebuf, uint8_t *binbuf)
{
	char *s;
	uint8_t v;
	uint8_t val;
	uint16_t arrptr;
	uint16_t bincnt;
	bincnt=0;
	s=linebuf;
	arrptr= 0;
	while(*s > 32)
	{
		val <<= 1;
		val |= (*s++) - '0';
		bincnt++;
		if (bincnt == 8)
		{
			bincnt = 0;
//			printf("Got word %02x pos %d\n",val,arrptr);
			binbuf[arrptr++] = val;
		}
	}
	return arrptr;
}

int load_jed(char *filename)
{
	FILE *jed_file;
	int ret;
	int i;
	int done;
	uint8_t binbuf[256];
	uint16_t binlen;
	char linebuf[256];
	char wrd[64];
	uint32_t bytesOut;
	char *s;
	char *p;
	uint32_t page_addr;
    jed_file = fopen(filename, "r");
	if (jed_file == NULL)
	{
		printf("Cannot open %s\n",filename);
		return -1;
	}
	done = 0;
	bytesOut = 0;
	while ((!feof(jed_file)) && (done == 0))
	{
		fgets(linebuf,256, jed_file);
		if (strlen(linebuf) > 0)
		{
			if (linebuf[0] == 'L')
			{
				s=&linebuf[1];
				p=wrd;
				while (*s > 32)	
				{
					*p++ = *s++;
				}
				*p = 0;
				page_addr = strtoul(wrd,0,16);
				if (page_addr == 0)
				{
					reset_config_addr();
					do 
					{
						fgets(linebuf,256, jed_file);
						if (strlen(linebuf) >= 128) 
						{
//							printf("Read in line len %d\n",strlen(linebuf));
							binlen=cvt_to_array(linebuf,binbuf);
							if (binlen > 0)
							{
								printf("Converted to array of size %d\n%s",binlen,linebuf);
								for (i=0; i<binlen; i++)
								{
									printf("%02X      ",binbuf[i]);
								}
								printf("\n");
								bytesOut += 16;
							}
							program_page(binbuf);
							wait_busy();
							//program_page();
						} else
						if (linebuf[0] == '*')
						{
							printf("Wrote\n");
							done=1;
						}
						else
						{
							printf("bad page data format\n");
							done=1;
						}
					} while ((strlen(linebuf)>127) && (done == 0));
				}
			}
		}
	}
	printf("Programmed %d bytes\n",bytesOut);
	fclose(jed_file);

}

int main(int argc, char *argv[])
{

	int length;
	unsigned char buffer[60] = {0};
	uint32_t devid;
	char fname[256];


	//----- OPEN THE I2C BUS -----
	char *filename = (char*)"/dev/i2c-1";
	if ((file_i2c = open(filename, O_RDWR)) < 0)
	{
		//ERROR HANDLING: you can check errno to see what went wrong
		printf("Failed to open the i2c bus");
		return -1;
	}

	int addr = I2C_ADDR;          //<<<<<The I2C address of the slave
	if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
	{
		printf("Failed to acquire bus access and/or talk to slave.\n");
		//ERROR HANDLING; you can check errno to see what went wrong
		return -1;
	}

	// read device id
	devid = read_device_id();
	printf("devid returned %08x\n",devid);

	if (argc > 1)
	{
		strcpy(fname, argv[1]);
		printf("filename is %s\n",fname);
		enable_config_offline();
		printf("Sending erase\n");
		erase(ERASE_CONGFIG_FLASH);
		wait_busy();
		printf("Done erasing\n");
		load_jed(fname);
		program_done();
		printf("MachXO2 status %08x\n",read_status());
		refresh();
	}


	return 0;
}
