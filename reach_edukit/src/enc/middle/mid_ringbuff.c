#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mid_mutex.h"
#include "mid_ringbuff.h"
//#include "log_common.h"
#define dprintf printf
#define MAX_BLOCK_SIZE (5*188)


static int VDRMGetDataSizeHelper(ReachRingBuffer *ringBuffer);
static int VDRMGetFreeSizeHelper(ReachRingBuffer *ringBuffer);
static int  vrdrm_check_buff_to_ts(char *buf, int size, int *leftmove);

/********************************************************
*   ��buffer���룬��ȡ�õ����ݣ�ֻȡǰ��188��������ݣ��������ݶ���
*
***********************************************************/
static int  vrdrm_check_buff_to_ts(char *buf, int size, int *leftmove)
{
#if 1
	*leftmove = 0;
	return 1;
#else
	int i = 0;
	int flag = 0 ;//flag = 1��ʾ���������TS���ݣ��������������̫С����ôֻȡ188���ֽ�

	while(i < 188 * 2 && i < size) {
		if(buf[i] != 0x47) {
			i ++;
			continue;
		}

		if(i < size - 188 * 2) {
			if(buf[i] == 0x47 && buf[i + 188] == 0x47 && buf[i + 188 * 2] == 0x47) {
				//*leftmove = i;
				flag = 1;
				break;
			}

			i ++ ;
		} else {
			break;
		}
	}

	*leftmove = i;
	return flag;
#endif
}

/*�õ���ǰ�Ŀɶ������ݴ�С*/
static int VDRMGetDataSizeHelper(ReachRingBuffer *ringBuffer)
{
	if(ringBuffer->writePos >= ringBuffer->readPos) {
		return ringBuffer->writePos - ringBuffer->readPos;
	} else {
		return ringBuffer->len - ringBuffer->readPos + ringBuffer->writePos;
	}
}
/*�õ����е�buffer��С*/
static int VDRMGetFreeSizeHelper(ReachRingBuffer *ringBuffer)
{
	int freeSize;
	freeSize = ringBuffer->len - VDRMGetDataSizeHelper(ringBuffer);
	return freeSize;
}


/*�����µ�ringbuffer**/

ReachRingBuffer *mid_create_ringbuff(int size, int highWater)
{
	ReachRingBuffer *ringBuffer = (ReachRingBuffer *)malloc(sizeof(ReachRingBuffer));

	if(ringBuffer) {
		ringBuffer->buf = malloc(size);
		ringBuffer->writePos = ringBuffer->readPos = 0;

		if(!ringBuffer->buf) {
			free(ringBuffer);
			ringBuffer = NULL;
		} else {
			ringBuffer->len = size;
			ringBuffer->highWater = highWater;
			ringBuffer->mutex = mid_mutex_create();
		}
	}

	return ringBuffer;
}

/*�ͷ�ringbuffer*/
void mid_free_ringbuff(ReachRingBuffer *ringBuffer)
{
	if(!ringBuffer) {
		return;
	}

	mid_mutex_destroy(&ringBuffer->mutex);
	free(ringBuffer->buf);
	free(ringBuffer);
}

/*д���ݽ�ȥ*/
int mid_push_ringbuff(ReachRingBuffer *ringBuffer, char *buf, int len)
{
	mid_mutex_lock(ringBuffer->mutex);
	//printf("\n");
	char *temp = NULL;

	if(VDRMGetFreeSizeHelper(ringBuffer) <= len) {
		mid_mutex_unlock(ringBuffer->mutex);
		printf("***** warnning Reach RingBuffer full\n");
		return -1;
	} else {
		//printf("\n");

		//ringBuffer�ĵײ��������ɸ����ݡ�
		if(ringBuffer->writePos + len - 1 < ringBuffer->len) {
			//printf("\n");
			//	sleep(10);
			temp = ringBuffer->buf + ringBuffer->writePos;
			//	printf("BUF=%p,len=%d==%d,%p\n",buf,len,ringBuffer->writePos,ringBuffer->buf);

			memcpy(temp, buf, len);
			//printf("\n");
			ringBuffer->writePos += len;

			//printf("\n");
			if(ringBuffer->writePos == ringBuffer->len) {
				ringBuffer->writePos = 0;
			}
		} else {
			//printf("\n");
			//ringBuffer�ĵײ��޷����ɸ����ݡ��Ƚ���������copy���ײ���
			int len1 = ringBuffer->len - ringBuffer->writePos;

			if(len1 > 0) {
				memcpy(ringBuffer->buf + ringBuffer->writePos, buf, len1);
			}

			//���ۻأ���ʣ�ಿ������copy��������
			if(len - len1 > 0) {
				memcpy(ringBuffer->buf, buf + len1, len - len1);
			}

			ringBuffer->writePos = len - len1;
		}

		//printf("\n");
	}

	//printf("\n");
	mid_mutex_unlock(ringBuffer->mutex);

	return 0;
}

/*���ļ�������֮�󣬱Ƚ�needLen��readlen*/
int mid_get_ringbuff(ReachRingBuffer *ringBuffer, char *buf, int needLen, int *readLen)
{
	int leftmove = 0;
	int tsflag = 0;
	int dataSize;
	char *dest;
	mid_mutex_lock(ringBuffer->mutex);

	if((dataSize = VDRMGetDataSizeHelper(ringBuffer)) >= needLen) {
		*readLen = needLen;
	} else {
		*readLen = dataSize;
	}

	if(*readLen == 0) {
		mid_mutex_unlock(ringBuffer->mutex);
		return -1;
	} else {
		if(ringBuffer->readPos < ringBuffer->writePos) {
			if(ringBuffer->writePos - ringBuffer->readPos >= *readLen) {

				dest = ringBuffer->buf + ringBuffer->readPos;
				/**ȡ��������ǰ188�ֽ��з�TS�ĳ���**/
				tsflag = vrdrm_check_buff_to_ts(dest, *readLen, &leftmove);

				if(leftmove > 0) {
					dprintf("1tsflag =%d,leftmove == %d =0x%02x=%d\n", tsflag, leftmove, dest[0], *readLen);
				}

				//				if(tsflag == 0 || dest[leftmove] != 0x47)
				//					*readLen = 188 ;
				//				else{
				*readLen = *readLen - leftmove;
				//	*readLen = *readLen - *readLen%8192;/*ȷ���κ�ʱ����188�ı���*/
				//				}
#if 0

				if(tsflag != 1) {
					/*���û��ȡ��188��ͷ����ֻȡ376������*/
					if(*readLen > 188 * 2) {
						*readLen = 188 * 2;
					}
				}

#endif

				if(*readLen != 0) {
					memcpy(buf, &dest[leftmove], *readLen);
				}

				ringBuffer->readPos += *readLen + leftmove;

				if(ringBuffer->readPos == ringBuffer->len) {
					ringBuffer->readPos = 0;
					//printf("====== readPos=0\n");
				}

				//printf("------- readPos=%d\n", ringBuffer->readPos);
			} else {
				printf("ring buffer is error 1\n");
			}
		} else {
			int len1 = ringBuffer->len - ringBuffer->readPos;

			if(len1 >= *readLen) {
				dest = ringBuffer->buf + ringBuffer->readPos;

				/**ȡ��������ǰ188�ֽ��з�TS�ĳ���**/
				tsflag = vrdrm_check_buff_to_ts(dest, *readLen, &leftmove);

				if(leftmove > 0) {
					dprintf("2tsflag =%d,leftmove == %d =0x%02x=%d\n", tsflag, leftmove, dest[0], *readLen);    //				if(tsflag == 0 || dest[leftmove] != 0x47)
				}

				//					*readLen = 188 ;
				//				else{
				*readLen = *readLen - leftmove;

				//	*readLen = *readLen - *readLen%8192;/*ȷ���κ�ʱ����188�ı���*/
				//		}
				if(*readLen != 0) {
					memcpy(buf, &dest[leftmove], *readLen);
				}

				ringBuffer->readPos += *readLen + leftmove;


				if(ringBuffer->readPos == ringBuffer->len) {
					ringBuffer->readPos = 0;
				}
			} else {
				dest = ringBuffer->buf + ringBuffer->readPos;
				/**ȡ��������ǰ188�ֽ��з�TS�ĳ���**/
				/*��ȷ������������188��������*/
				int templen = len1;
				tsflag = vrdrm_check_buff_to_ts(dest, templen, &leftmove);

				if(leftmove > 0) {
					dprintf("3tsflag =%d,leftmove == %d =0x%02x=%d\n", tsflag, leftmove, dest[0], templen);
				}

				if((templen - leftmove) != 0) {
					memcpy(buf, &dest[leftmove], templen - leftmove);
				}

				templen = templen - leftmove;

				if(*readLen - len1 > 0) {
					dest = ringBuffer->buf;
					memcpy(buf + templen, dest, *readLen - len1);
				}

				*readLen = *readLen - leftmove;
				ringBuffer->readPos = *readLen - len1;

				if(ringBuffer->readPos == ringBuffer->len) {
					ringBuffer->readPos = 0;
					//printf("---------------- readPos=0\n");
				}
			}

			//printf("len1=%d, *readLen-len1=%d\n", len1, *readLen-len1);
			//printf("+++++++ readPos=%d\n", ringBuffer->readPos);
		}

	}

	mid_mutex_unlock(ringBuffer->mutex);
	return 0;
}




/*�ⲿ�ӿڵ��ã��õ�SIZE*/
int mid_get_usesize_ringbuff(ReachRingBuffer *ringBuffer)
{
	int dataSize;
	mid_mutex_lock(ringBuffer->mutex);
	dataSize = VDRMGetDataSizeHelper(ringBuffer);
	mid_mutex_unlock(ringBuffer->mutex);
	return dataSize;
}



/*�ⲿ�ӿڵ��ã��õ����е�buffer��С*/
int mid_get_freesize_ringbuff(ReachRingBuffer *ringBuffer)
{
	int freeSize;
	mid_mutex_lock(ringBuffer->mutex);
	freeSize = ringBuffer->len - VDRMGetDataSizeHelper(ringBuffer);
	mid_mutex_unlock(ringBuffer->mutex);
	return freeSize;
}

/*���ringbuffer*/
void mid_clear_ringbuff(ReachRingBuffer *ringBuffer)
{
	mid_mutex_lock(ringBuffer->mutex);

	if(ringBuffer) {
		ringBuffer->writePos = 0;
		ringBuffer->readPos = 0;

		/*�޸ģ���̨�����BUFFER���ͷ��ڴ�*/
		if(ringBuffer->buf != NULL) {
			free(ringBuffer->buf);
			dprintf("***************######now free the vmdrm ringbuffer \n");
		}

		ringBuffer->buf = NULL;
	}

	mid_mutex_unlock(ringBuffer->mutex);
}

int mid_reset_ringbuff(ReachRingBuffer *ringBuffer)
{
	int ret = 0;
	mid_mutex_lock(ringBuffer->mutex);

	if(ringBuffer) {
		if(ringBuffer->buf == NULL) {
			ringBuffer->buf = malloc(ringBuffer->len);

			if(ringBuffer->buf == NULL) {
				printf("*********error! there is not enough buffer for vmdrm_ringbuff\n");
				ret = -1;
			}

			dprintf("************#######now reset the vmdrm ringbuffer \n");
		}

		ringBuffer->writePos = 0;
		ringBuffer->readPos = 0;
	}

	mid_mutex_unlock(ringBuffer->mutex);
	return ret;
}

/*ringbuffer����ĳ����ֵ����Ϊ��*/
int mid_is_full_ringbuff(ReachRingBuffer *ringBuffer)
{
	if(VDRMGetFreeSizeHelper(ringBuffer) <= MAX_BLOCK_SIZE) {
		return 1;
	} else {
		return 0;
	}
}

int mid_is_highwater_ringbuff(ReachRingBuffer *ringBuffer)
{
	int ret;
	mid_mutex_lock(ringBuffer->mutex);

	if(VDRMGetDataSizeHelper(ringBuffer) >= (ringBuffer->len * ringBuffer->highWater / 100.0f)) {
		ret = 1;
	} else {
		ret = 0;
	}

	mid_mutex_unlock(ringBuffer->mutex);
	return ret;
}





