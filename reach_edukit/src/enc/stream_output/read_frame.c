#ifdef HAVE_RTSP_MODULE



#include <stdio.h>
#include "log.h"
#include "log_common.h"

typedef struct bs_s

{

	unsigned char *p_start;                // 缓冲区首地址

	unsigned char *p;                         // 缓冲区当前的读写指针

	unsigned char *p_end;                 // 缓冲区尾地址

	int     i_left;                // p所指字节当前还有多少比特可读写

	int     i_bits_encoded;   /* RD only */

} bs_t;

static inline void bs_init(bs_t *s, void *p_data, int i_data)
{
	s->p_start = (unsigned char *)p_data;
	s->p       = (unsigned char *)p_data;
	s->p_end   = s->p + i_data;
	s->i_left  = 8;
}
static inline unsigned int bs_read(bs_t *s, int i_count)
{
	static unsigned int i_mask[33] = {0x00,
	                                  0x01,      0x03,      0x07,      0x0f,
	                                  0x1f,      0x3f,      0x7f,      0xff,
	                                  0x1ff,     0x3ff,     0x7ff,     0xfff,
	                                  0x1fff,    0x3fff,    0x7fff,    0xffff,
	                                  0x1ffff,   0x3ffff,   0x7ffff,   0xfffff,
	                                  0x1fffff,  0x3fffff,  0x7fffff,  0xffffff,
	                                  0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff,
	                                  0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff
	                                 };
	int      i_shr;
	unsigned int i_result = 0;

	while(i_count > 0) {
		if(s->p >= s->p_end) {
			break;
		}

		if((i_shr = s->i_left - i_count) >= 0) {

			i_result |= (*s->p >> i_shr)&i_mask[i_count];
			s->i_left -= i_count;

			if(s->i_left == 0) {
				s->p++;
				s->i_left = 8;
			}

			return(i_result);
		} else {

			i_result |= (*s->p & i_mask[s->i_left]) << -i_shr;
			i_count  -= s->i_left;
			s->p++;
			s->i_left = 8;
		}
	}

	return(i_result);
}

static inline unsigned int bs_read1(bs_t *s)
{
	if(s->p < s->p_end) {
		unsigned int i_result;

		s->i_left--;
		i_result = (*s->p >> s->i_left) & 0x01;

		if(s->i_left == 0) {
			s->p++;
			s->i_left = 8;
		}

		return i_result;
	}

	return 0;
}

static inline int bs_read_ue(bs_t *s)
{
	int i = 0;

	while(bs_read1(s) == 0 && s->p < s->p_end && i < 32) {
		i++;
	}

	return((1 << i) - 1 + bs_read(s, i));
}

static inline int bs_read_se(bs_t *s)
{
	int val = bs_read_ue(s);

	return val & 0x01 ? (val + 1) / 2 : -(val / 2);
}

int read_gaps_in_frame(unsigned char *p,  int *width,  int *hight, unsigned int len)
{
	unsigned int type;
	bs_t s;

	PRINTF("%d\n", p[1]);

	if(p[1] == 100) {   //HP
		bs_init(&s, &p[5], len - 5);
	} else if(p[1] == 66) { //BP
		bs_init(&s, &p[4], len - 4);
		/* 获取seq_parameter_set_id */
		PRINTF("%d\n", bs_read_ue(&s));
	} else {
		*width = 0;
		*hight = 0;
		return 0;
	}



	/* 获取log2_max_frame_num_minus4 */
	PRINTF("%d\n", bs_read_ue(&s));
	type =	bs_read_ue(&s);
	PRINTF("%d\n", type);

	/* pic_order_cnt_type = 0 */
	if(0 == type) {
		/* 获取log2_max_pic_order_cnt.. */
		PRINTF("%d\n", bs_read_ue(&s));

		/* 获取 num_ref_frames */
		PRINTF("%d\n", bs_read_ue(&s));
		s.i_left = s.i_left - 1;
		*width =  bs_read_ue(&s);
		*hight =  bs_read_ue(&s);
		*width = (*width + 1) * 16;
		*hight = (*hight + 1) * 16;
	} else if(1 == type) {
		unsigned int num_ref_frames_in_pic;
		int i = 0;
		/* data_pic_order_always_zero_flag */
		s.i_left = s.i_left - 1;
		bs_read_se(&s);
		bs_read_se(&s);
		num_ref_frames_in_pic = bs_read_ue(&s);

		for(i = 0; i < num_ref_frames_in_pic; i++) {
			bs_read_se(&s);
		}

		bs_read_ue(&s);
		s.i_left = s.i_left - 1;
		*width =  bs_read_ue(&s);
		*hight =  bs_read_ue(&s);
	} else {
		PRINTF("dec bit stream is failure!\n");
	}

	return 0;
}

#endif