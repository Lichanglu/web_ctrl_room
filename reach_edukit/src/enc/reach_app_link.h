#ifndef	__REACH_LINK_H__
#define	__REACH_LINK_H__

#include <osa.h>

#include <link_api/system.h>
#include <link_api/captureLink.h>
#include <link_api/displayLink.h>
#include <ti_vdis_timings.h>
#include <mcfw/src_linux/mcfw_api/reach_system_priv.h>

#include <osa_que.h>
#include <osa_mutex.h>
#include <osa_thr.h>
#include <osa_sem.h>

#include "stdint.h"

#include "reach_ipcbit.h"


static inline void setVpssCaptureParams(CaptureLink_CreateParams *pCapturePrm,
                                        Int32 numVipInst, UInt32 *vipInstId, Int32 *nextLinkId)
{
	int i = 0;
	CaptureLink_VipInstParams		*pCaptureInstPrm;
	CaptureLink_OutParams			*pCaptureOutPrm;
	pCapturePrm->numVipInst               = numVipInst;
	pCapturePrm->tilerEnable              = FALSE;
	pCapturePrm->fakeHdMode               = TRUE;
	pCapturePrm->enableSdCrop             = FALSE;
	pCapturePrm->doCropInCapture          = FALSE;
	pCapturePrm->numBufsPerCh             = 6;
	pCapturePrm->maxBlindAreasPerCh		  = 4;

	for(i = 0; i < numVipInst; i ++) {
		pCaptureInstPrm                     = &(pCapturePrm->vipInst[i]);
		pCaptureInstPrm->vipInstId          = vipInstId[i];//SYSTEM_CAPTURE_INST_VIP0_PORTA;
		pCaptureInstPrm->inDataFormat       = SYSTEM_DF_YUV422P;
		pCaptureInstPrm->standard           = SYSTEM_STD_1080P_60;
		pCaptureInstPrm->numOutput          = 1;

		pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
		pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV420SP_UV;
		pCaptureOutPrm->scEnable            = FALSE;
		pCaptureOutPrm->scOutWidth          = 1920;
		pCaptureOutPrm->scOutHeight         = 1080;
		pCaptureOutPrm->outQueId            = i;
		pCapturePrm->outQueParams[i].nextLink = nextLinkId[i];
	}
}

static inline void setVpssDupParams(DupLink_CreateParams *pDupPrm,
                                    Int32 prevLinkId, Int32 prevLinkQueId,
                                    Int32 numOutQue, Int32 *nextLinkId)
{
	Int32 i = 0;
	pDupPrm->inQueParams.prevLinkId		= prevLinkId;
	pDupPrm->inQueParams.prevLinkQueId = prevLinkQueId;
	pDupPrm->numOutQue 				= numOutQue;

	for(i = 0; i < numOutQue; i ++) {
		pDupPrm->outQueParams[i].nextLink	= nextLinkId[i];
	}

	pDupPrm->notifyNextLink			= TRUE;
}

static inline void setVpssDeiParams(DeiLink_CreateParams *pDeiPrm,
                                    Int32 prevLinkId, Int32 prevLinkQueId,
                                    Int32 nextLinkId)
{
	//only for input 1 dei
	//in: 1 que in and 1 ch in .  ch = {in1}
	//out: 1 que out and 1 ch out .ch= {in1}  //may be no input ,if the input1 is p siginal
	pDeiPrm->inQueParams.prevLinkId 					= prevLinkId;
	pDeiPrm->inQueParams.prevLinkQueId					= prevLinkQueId;
	pDeiPrm->outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink = nextLinkId;
	pDeiPrm->enableOut[DEI_LINK_OUT_QUE_DEI_SC] 		= FALSE;
	pDeiPrm->enableOut[DEI_LINK_OUT_QUE_VIP_SC] 		= TRUE; //只有队列1 有数据
	pDeiPrm->tilerEnable									= FALSE;
	pDeiPrm->comprEnable									= FALSE;
	pDeiPrm->setVipScYuv422Format							= FALSE;
	pDeiPrm->enableDeiForceBypass							= FALSE;
	pDeiPrm->enableLineSkipSc								= FALSE;

	pDeiPrm->inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC]		= 60;
	pDeiPrm->outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC]		= 30;
	pDeiPrm->inputDeiFrameRate = 60;
	pDeiPrm->outputDeiFrameRate = 60;

	pDeiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode = DEI_SCALE_MODE_ABSOLUTE;
	pDeiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].absoluteResolution.outWidth = 1920;
	pDeiPrm->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].absoluteResolution.outHeight = 1080;
	pDeiPrm->numBufsPerCh[0] = 3;
	pDeiPrm->numBufsPerCh[1] = 3;
	pDeiPrm->numBufsPerCh[2] = 3;
}

static inline void setVpssNullSrcParams(NullSrcLink_CreateParams *pNullSrcPrm,
                                        Int32 numCh, Int32 nextLinkId)
{
	Int32 i = 0;
	pNullSrcPrm->outQueParams.nextLink = nextLinkId;
	pNullSrcPrm->tilerEnable = FALSE;
	pNullSrcPrm->timerPeriod = 33;		/* 10 fps */
	pNullSrcPrm->inputInfo.numCh = numCh;

	for(i = 0; i < numCh; i ++) {
		pNullSrcPrm->inputInfo.chInfo[i].bufType = SYSTEM_BUF_TYPE_VIDFRAME;
		pNullSrcPrm->inputInfo.chInfo[i].memType = SYSTEM_MT_NONTILEDMEM;
		pNullSrcPrm->inputInfo.chInfo[i].codingformat = 0;
		pNullSrcPrm->inputInfo.chInfo[i].dataFormat = SYSTEM_DF_YUV420SP_UV;
		pNullSrcPrm->inputInfo.chInfo[i].scanFormat = 1;
		pNullSrcPrm->inputInfo.chInfo[i].width = 1920;
		pNullSrcPrm->inputInfo.chInfo[i].height = 1080;
		pNullSrcPrm->inputInfo.chInfo[i].startX = 0;
		pNullSrcPrm->inputInfo.chInfo[i].startY = 0;
		pNullSrcPrm->inputInfo.chInfo[i].pitch[0] = 1920;
		pNullSrcPrm->inputInfo.chInfo[i].pitch[1] = 1920;   //???pitch only one ?? zm
	}
}

static inline void setVpssMergeParams(MergeLink_CreateParams *pMergePrm,
                                      Int32 numInQue, Int32 *prevLinkId,
                                      Int32 *prevLinkQueId, Int32 nextLinkId)
{
	int i = 0;
	pMergePrm->numInQue						= numInQue;
	pMergePrm->outQueParams.nextLink		= nextLinkId;
	pMergePrm->notifyNextLink				= TRUE;

	for(i = 0; i < numInQue; i ++) {
		printf("[setVpssMergeParams][%d] prevLinkId:[%d] prevLinkQueId:[%d]\n",
		       i, prevLinkId[i], prevLinkQueId[i]);
		pMergePrm->inQueParams[i].prevLinkId		= prevLinkId[i];
		pMergePrm->inQueParams[i].prevLinkQueId	= prevLinkQueId[i];
	}
}

static inline void setVpssSelectParams(SelectLink_CreateParams *pSelectPrm,
                                       Int32 prevLinkId, Int32 prevLinkQueId,
                                       Int32 numOutQue, Int32 *nextLinkId,
                                       Int32 *numOutCh, char *inChNum)
{
	printf("[setVpssSelectParams] start ...\n");
	Int32 i = 0;
	Int32 j = 0;
	Int32 k = 0;
	pSelectPrm->inQueParams.prevLinkId	  = prevLinkId;
	pSelectPrm->inQueParams.prevLinkQueId = prevLinkQueId;
	pSelectPrm->numOutQue = numOutQue;

	for(i = 0; i < numOutQue; i ++) {
		pSelectPrm->outQueParams[i].nextLink = nextLinkId[i];
		pSelectPrm->outQueChInfo[i].numOutCh = numOutCh[i];
		printf("[setVpssSelectParams] numOutCh:[%d]\n", numOutCh[i]);

		for(j = 0; j < numOutCh[i]; j ++) {
			printf("[setVpssSelectParams] i = %d j = %d inChNum[k]:[%d]\n", i, j, inChNum[k]);
			pSelectPrm->outQueChInfo[i].inChNum[j] = inChNum[k];
			k ++;
		}
	}
}

static inline void setVpssSclrParams(SclrLink_CreateParams *pSclrPrm,
                                     Int32 prevLinkId, Int32 prevLinkQueId,
                                     Int32 nextLinkId, UInt32 scaleMode,
                                     UInt32 outWidth, UInt32 outHeight)
{
	pSclrPrm->inQueParams.prevLinkId			  = prevLinkId;
	pSclrPrm->inQueParams.prevLinkQueId		  = prevLinkQueId;
	pSclrPrm->pathId							  = SCLR_LINK_SC5;
	pSclrPrm->outQueParams.nextLink			  = nextLinkId;
	pSclrPrm->tilerEnable						  = FALSE;
	pSclrPrm->enableLineSkipSc 				  	  = FALSE;
	pSclrPrm->inputFrameRate					  = 60;
	pSclrPrm->outputFrameRate					  = 30;
	pSclrPrm->scaleMode						  = scaleMode;
	pSclrPrm->numBufsPerCh 				  = 4;

	if(scaleMode == pSclrPrm->scaleMode) {
		pSclrPrm->outScaleFactor.absoluteResolution.outWidth = outWidth;
		pSclrPrm->outScaleFactor.absoluteResolution.outHeight = outHeight;
	} else {
		pSclrPrm->outScaleFactor.ratio.widthRatio.numerator	= 1;
		pSclrPrm->outScaleFactor.ratio.widthRatio.denominator	= 1;
		pSclrPrm->outScaleFactor.ratio.heightRatio.numerator	= 1;
		pSclrPrm->outScaleFactor.ratio.heightRatio.denominator = 1;
	}
}

static inline void setHDMIDisplayParams(DisplayLink_CreateParams *pDisplayPrm,
                                        Int32 prevLinkId, Int32 prevLinkQueId,
                                        UInt32 displayRes)
{
	VDIS_PARAMS_S					vdisParams;

	pDisplayPrm->inQueParams[0].prevLinkId		= prevLinkId;
	pDisplayPrm->inQueParams[0].prevLinkQueId	= prevLinkQueId;
	pDisplayPrm->displayRes					= displayRes;

	dis_params_init(&vdisParams);
	vdisParams.numChannels = 1;
	vdisParams.deviceParams[SYSTEM_DC_VENC_HDMI].resolution 	= displayRes;
	vdisParams.enableLayoutGridDraw = FALSE;
	vpss_displayctrl_init(&vdisParams);
}

static inline void setVpssNsfParams(NsfLink_CreateParams *pNsfPrm,
                                    Int32 prevLinkId, Int32 prevLinkQueId,
                                    Int32 nextLinkId)
{
	pNsfPrm->inQueParams.prevLinkId   = prevLinkId;
	pNsfPrm->inQueParams.prevLinkQueId = prevLinkQueId;
	pNsfPrm->bypassNsf 			   = FALSE;
	pNsfPrm->tilerEnable			   = FALSE;
	pNsfPrm->numOutQue 			   = 1;
	pNsfPrm->outQueParams[0].nextLink = nextLinkId;
}

static inline void setIpcFramesOutVpssToDspParams(IpcFramesOutLinkRTOS_CreateParams  *pIpcFramesOutVpssToDspPrm,
        Int32 prevLinkId, Int32 prevLinkQueId, Int32 nextLinkId, Int32 processLinkId)
{
	pIpcFramesOutVpssToDspPrm->baseCreateParams.inQueParams.prevLinkId = prevLinkId;
	pIpcFramesOutVpssToDspPrm->baseCreateParams.inQueParams.prevLinkQueId = prevLinkQueId;
	pIpcFramesOutVpssToDspPrm->baseCreateParams.notifyPrevLink = TRUE;
	pIpcFramesOutVpssToDspPrm->baseCreateParams.numOutQue = 1;
	pIpcFramesOutVpssToDspPrm->baseCreateParams.outQueParams[0].nextLink = nextLinkId;
	pIpcFramesOutVpssToDspPrm->baseCreateParams.notifyNextLink = TRUE;
	pIpcFramesOutVpssToDspPrm->baseCreateParams.processLink = processLinkId ;
	pIpcFramesOutVpssToDspPrm->baseCreateParams.notifyProcessLink = TRUE;
	pIpcFramesOutVpssToDspPrm->baseCreateParams.noNotifyMode = FALSE;
}

static inline void setIpcFramesInDspParams(IpcFramesInLinkRTOS_CreateParams *pIpcFramesInDspPrm,
        Int32 prevLinkId, Int32 prevLinkQueId, Int32 nextLinkId)
{
	pIpcFramesInDspPrm->baseCreateParams.inQueParams.prevLinkId = prevLinkId;
	pIpcFramesInDspPrm->baseCreateParams.inQueParams.prevLinkQueId = prevLinkQueId;
	pIpcFramesInDspPrm->baseCreateParams.numOutQue   = 1;
	pIpcFramesInDspPrm->baseCreateParams.outQueParams[0].nextLink = nextLinkId;
	pIpcFramesInDspPrm->baseCreateParams.notifyPrevLink = TRUE;
	pIpcFramesInDspPrm->baseCreateParams.notifyNextLink = TRUE;
	pIpcFramesInDspPrm->baseCreateParams.noNotifyMode   = FALSE;
}

static inline void setOsdDspTrackAlgparams(AlgLink_CreateParams *pOsdDspAlgPrm,
        Int32 prevLinkId, Int32 prevLinkQueId)
{
	pOsdDspAlgPrm->inQueParams.prevLinkId = prevLinkId;
	pOsdDspAlgPrm->inQueParams.prevLinkQueId = prevLinkQueId;
	pOsdDspAlgPrm->outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink = SYSTEM_LINK_ID_INVALID;
	pOsdDspAlgPrm->enableOSDAlg	     = FALSE;
	pOsdDspAlgPrm->enableSCDAlg	     = FALSE;

	pOsdDspAlgPrm->enableSTUTRACKAlg = TRUE;
	pOsdDspAlgPrm->enableTRACKAlg	 = TRUE;

	if(pOsdDspAlgPrm->enableTRACKAlg) {
		pOsdDspAlgPrm->trackCreateParams.ChToStream[1] = 0;
	}

	if(pOsdDspAlgPrm->enableSTUTRACKAlg) {
		pOsdDspAlgPrm->stutrackCreateParams.ChToStream[0] = 0;
	}

}

typedef struct encNum_ {
	Int32 jpeg;
	Int32 h264;
} encNum_t;

typedef struct videoEncoderLink_ {
	encNum_t			sEncNum;
	ipcoutm3_struct		ipc_outvpss_Link;
	ipcinm3_struct			ipc_invideo_Link;
	enc_struct			encLink;
	ipcbit_outvideo_struct	ipcbit_outvideo_Link;
	ipcbit_inhost_struct		ipcbit_inhost_Link;
	bits_user_param 		bitsparam;
	Int32(*bits_callback_fxn)(bits_user_param *hndle_param);
} videoEncoderLink_t;

static inline void setVideoEncoderParams(videoEncoderLink_t *pEnc, Int32 prevLinkId, Int32 prevLinkQueId)
{
	Int32 queId = 0;
	Void *bit_handle = NULL;
	IpcLink_CreateParams				*pIpcOutVpssPrm = &(pEnc->ipc_outvpss_Link.create_params);
	IpcLink_CreateParams				*pIpcInVideoPrm = &(pEnc->ipc_invideo_Link.create_params);
	EncLink_CreateParams				*pEncPrm = &(pEnc->encLink.create_params);
	IpcBitsOutLinkRTOS_CreateParams	*pIpcBitsOutVideoPrm = &(pEnc->ipcbit_outvideo_Link.create_params);
	IpcBitsInLinkHLOS_CreateParams		*pIpcBitsInHostPrm = &(pEnc->ipcbit_inhost_Link.create_params);

	pIpcOutVpssPrm->inQueParams.prevLinkId		= prevLinkId;
	pIpcOutVpssPrm->inQueParams.prevLinkQueId	= prevLinkQueId;
	pIpcOutVpssPrm->numOutQue					= 1;
	pIpcOutVpssPrm->outQueParams[0].nextLink	= pEnc->ipc_invideo_Link.link_id;
	pIpcOutVpssPrm->notifyNextLink				= TRUE;
	pIpcOutVpssPrm->notifyPrevLink				= TRUE;
	pIpcOutVpssPrm->noNotifyMode				= FALSE;

	pIpcInVideoPrm->inQueParams.prevLinkId		= pEnc->ipc_outvpss_Link.link_id;
	pIpcInVideoPrm->inQueParams.prevLinkQueId	= 0;
	pIpcInVideoPrm->numOutQue					= 1;
	pIpcInVideoPrm->outQueParams[0].nextLink	= pEnc->encLink.link_id;
	pIpcInVideoPrm->notifyNextLink				= TRUE;
	pIpcInVideoPrm->notifyPrevLink				= TRUE;
	pIpcInVideoPrm->noNotifyMode				= FALSE;

	for(queId = 0; queId < pEnc->sEncNum.h264; queId++) {

		pEncPrm->chCreateParams[queId].format	= IVIDEO_H264HP;//IVIDEO_H264HP;
		pEncPrm->chCreateParams[queId].profile	= IH264_HIGH_PROFILE;//IH264_HIGH_PROFILE
		pEncPrm->chCreateParams[queId].dataLayout = IVIDEO_FIELD_SEPARATED;
		pEncPrm->chCreateParams[queId].fieldMergeEncodeEnable  = FALSE;
		pEncPrm->chCreateParams[queId].enableAnalyticinfo = FALSE;
		pEncPrm->chCreateParams[queId].enableWaterMarking = FALSE;
		pEncPrm->chCreateParams[queId].maxBitRate = 2000 * 1000;

		//设置编码场景，比如高质量还是高速度，默认中速度中质量
		pEncPrm->chCreateParams[queId].encodingPreset = XDM_USER_DEFINED;
		pEncPrm->chCreateParams[queId].rateControlPreset = IVIDEO_USER_DEFINED;

		pEncPrm->chCreateParams[queId].enableHighSpeed = 0;
		pEncPrm->chCreateParams[queId].enableSVCExtensionFlag = 0;
		pEncPrm->chCreateParams[queId].numTemporalLayer = 1;

		//只有encodingPreset == XDM_USER_DEFINED,下面设置才生效
		if(pEncPrm->chCreateParams[queId].encodingPreset == XDM_USER_DEFINED) {
			pEncPrm->chCreateParams[queId].enableHighSpeed = 0;
		}

		pEncPrm->chCreateParams[queId].enableSVCExtensionFlag = 0;
		pEncPrm->chCreateParams[queId].numTemporalLayer = 0;
		pEncPrm->chCreateParams[queId].defaultDynamicParams.intraFrameInterval = 120;
		pEncPrm->chCreateParams[queId].defaultDynamicParams.targetBitRate = 2000 * 1000; //video_param.sBitrate*1000;
		pEncPrm->chCreateParams[queId].defaultDynamicParams.interFrameInterval = 1;

		pEncPrm->chCreateParams[queId].defaultDynamicParams.mvAccuracy = IVIDENC2_MOTIONVECTOR_QUARTERPEL ; //IVIDENC2_MOTIONVECTOR_QUARTERPEL;
		pEncPrm->chCreateParams[queId].defaultDynamicParams.inputFrameRate = 60;//video_param.nFrameRate;

		//只有IVIDEO_USER_DEFINED才生效
		pEncPrm->chCreateParams[queId].defaultDynamicParams.rcAlg = IH264_RATECONTROL_PRC;

		/*qpMax与qpMin取值范围(0-51)其值越小视频质量越高但相应码率也会有所提高故其值也不可太小*/
		pEncPrm->chCreateParams[queId].defaultDynamicParams.qpMin = 15;
		pEncPrm->chCreateParams[queId].defaultDynamicParams.qpMax = 48;
		pEncPrm->chCreateParams[queId].defaultDynamicParams.qpInit = -1;

		pEncPrm->chCreateParams[queId].defaultDynamicParams.vbrDuration = 8;
		pEncPrm->chCreateParams[queId].defaultDynamicParams.vbrSensitivity = 0;

		//只有IVIDEO_USER_DEFINED才生效
		if(pEncPrm->chCreateParams[queId].rateControlPreset == IVIDEO_USER_DEFINED) {
			pEncPrm->chCreateParams[queId].defaultDynamicParams.rcAlg = IH264_RATECONTROL_PRC;
		}

	}

#ifdef HAVE_JPEG

	for(queId = pEnc->sEncNum.h264; queId < pEnc->sEncNum.h264 + pEnc->sEncNum.jpeg; queId++) {

		EncLink_ChCreateParams 	 *pLinkChPrm = &(pEncPrm->chCreateParams[queId]);
		EncLink_ChDynamicParams	*pLinkDynPrm = &pLinkChPrm->defaultDynamicParams;

		//pEncPrm->chCreateParams[queId].format	= IVIDEO_MJPEG;
		//pEncPrm->chCreateParams[queId].encodingPreset = XDM_DEFAULT;

		pLinkChPrm->format                 = IVIDEO_MJPEG;
		pLinkChPrm->profile                = 0;
		pLinkChPrm->dataLayout             = IVIDEO_FIELD_SEPARATED;
		pLinkChPrm->fieldMergeEncodeEnable = FALSE;
		pLinkChPrm->enableAnalyticinfo     = 0;
		pLinkChPrm->enableWaterMarking     = 0;
		pLinkChPrm->maxBitRate             = 0;
		pLinkChPrm->encodingPreset         = 0;
		pLinkChPrm->rateControlPreset      = 0;
		pLinkChPrm->enableSVCExtensionFlag = 0;
		pLinkChPrm->numTemporalLayer       = 0;

		pLinkDynPrm->intraFrameInterval    = 0;
		pLinkDynPrm->targetBitRate         = 100 * 1000;
		pLinkDynPrm->interFrameInterval    = 0;
		pLinkDynPrm->mvAccuracy            = 0;
		pLinkDynPrm->inputFrameRate        = 1;//pDynPrm->inputFrameRate;
		pLinkDynPrm->qpMin                 = 0;
		pLinkDynPrm->qpMax                 = 0;
		pLinkDynPrm->qpInit                = -1;
		pLinkDynPrm->vbrDuration           = 0;
		pLinkDynPrm->vbrSensitivity        = 0;

	}

#endif

	pEncPrm->numBufPerCh[0] = 4;
	pEncPrm->numBufPerCh[1] = 4;
	pEncPrm->numBufPerCh[2] = 4;
	pEncPrm->numBufPerCh[3] = 4;
	pEncPrm->inQueParams.prevLinkId = pEnc->ipc_invideo_Link.link_id;
	pEncPrm->inQueParams.prevLinkQueId = 0;
	pEncPrm->outQueParams.nextLink = pEnc->ipcbit_outvideo_Link.link_id;

	pIpcBitsOutVideoPrm->baseCreateParams.inQueParams.prevLinkId	= pEnc->encLink.link_id;
	pIpcBitsOutVideoPrm->baseCreateParams.inQueParams.prevLinkQueId = 0;
	pIpcBitsOutVideoPrm->baseCreateParams.numOutQue 				= 1;
	pIpcBitsOutVideoPrm->baseCreateParams.outQueParams[0].nextLink	= pEnc->ipcbit_inhost_Link.link_id;
	pIpcBitsOutVideoPrm->baseCreateParams.noNotifyMode				= TRUE;
	pIpcBitsOutVideoPrm->baseCreateParams.notifyNextLink			= FALSE;
	pIpcBitsOutVideoPrm->baseCreateParams.notifyPrevLink			= TRUE;


	pIpcBitsInHostPrm->baseCreateParams.inQueParams.prevLinkId		= pEnc->ipcbit_outvideo_Link.link_id;
	pIpcBitsInHostPrm->baseCreateParams.inQueParams.prevLinkQueId	= 0;
	pIpcBitsInHostPrm->baseCreateParams.outQueParams[0].nextLink	= SYSTEM_LINK_ID_INVALID;
	pIpcBitsInHostPrm->baseCreateParams.noNotifyMode = TRUE;
	pIpcBitsInHostPrm->cbFxn = NULL;
	pIpcBitsInHostPrm->cbCtx = NULL;
	pIpcBitsInHostPrm->baseCreateParams.notifyNextLink = FALSE;
	pIpcBitsInHostPrm->baseCreateParams.notifyPrevLink = FALSE;

	ipcbit_create_bitsprocess_inst(&bit_handle,
	                               pEnc->ipcbit_inhost_Link.link_id,
	                               SYSTEM_LINK_ID_INVALID,
	                               pEnc->bits_callback_fxn,
	                               pEnc->bitsparam.appdata,
	                               NULL,
	                               NULL);

}

typedef struct videoDecoderLink_ {
	Int32 				iDecNum;
	ipcbit_outhost_struct	ipcbit_outhost_Link;
	ipcbit_invideo_struct	ipcbit_invideo_Link;
	dec_struct			decLink;
	ipcoutm3_struct		ipc_outvideo_Link;
	ipcinm3_struct			ipc_invpss_Link;
	bits_user_param 		bitsparam;
	Int32(*bits_callback_fxn)(bits_user_param *hndle_param);
} videoDecoderLink_t;

static inline void set_outhost_bits_info(System_LinkQueInfo *pInQueInfo, Int32 num_inst)
{
	UInt32 chId;
	System_LinkChInfo *pChInfo;

	if(pInQueInfo == NULL) {
		fprintf(stderr, "set_outhost_bits_info error, pInQueInfo is NULL!\n");
		return;
	}

	pInQueInfo->numCh = num_inst;

	for(chId = 0; chId < pInQueInfo->numCh; chId++) {
		pChInfo = &pInQueInfo->chInfo[chId];

		pChInfo->bufType		= 1;
		pChInfo->codingformat	= 7;
		pChInfo->dataFormat 	= 0;
		pChInfo->memType		= 0;
		pChInfo->startX 		= 0;
		pChInfo->startY 		= 0;
		pChInfo->width			= 1920;
		pChInfo->height 		= 1080;
		pChInfo->pitch[0]		= 1920;
		pChInfo->pitch[1]		= 1920;
		pChInfo->pitch[2]		= 0;
		pChInfo->scanFormat 	= SYSTEM_SF_PROGRESSIVE;
	}

}

static inline void setVideoDecoderParams(videoDecoderLink_t *pDec, Int32 nextLinkId)
{
	Void *bit_handle = NULL;
	Int32 queId = 0;

	IpcBitsOutLinkHLOS_CreateParams 	*pIpcBitsOutHostPrm = &(pDec->ipcbit_outhost_Link.create_params);
	IpcBitsInLinkRTOS_CreateParams		*pIpcBitsInVideoPrm = &(pDec->ipcbit_invideo_Link.create_params);
	DecLink_CreateParams				*pDecPrm = &(pDec->decLink.create_params);
	IpcLink_CreateParams				*pIpcOutVideoPrm = &(pDec->ipc_outvideo_Link.create_params);
	IpcLink_CreateParams				*pIpcInVpssPrm = &(pDec->ipc_invpss_Link.create_params);

	ipcbit_create_bitsprocess_inst(&bit_handle,
	                               SYSTEM_LINK_ID_INVALID,
	                               pDec->ipcbit_outhost_Link.link_id,
	                               NULL, NULL,
	                               pDec->bits_callback_fxn,
	                               NULL);

	pIpcBitsOutHostPrm->baseCreateParams.inQueParams.prevLinkId	= SYSTEM_LINK_ID_INVALID;
	pIpcBitsOutHostPrm->baseCreateParams.inQueParams.prevLinkQueId = 0;
	pIpcBitsOutHostPrm->baseCreateParams.numOutQue					= 1;
	pIpcBitsOutHostPrm->baseCreateParams.outQueParams[0].nextLink	= pDec->ipcbit_invideo_Link.link_id;
	pIpcBitsOutHostPrm->baseCreateParams.notifyNextLink				= FALSE;
	pIpcBitsOutHostPrm->baseCreateParams.notifyPrevLink				= FALSE;
	pIpcBitsOutHostPrm->baseCreateParams.noNotifyMode				= TRUE;
	set_outhost_bits_info(&pIpcBitsOutHostPrm->inQueInfo, pDec->iDecNum);

	pIpcBitsInVideoPrm->baseCreateParams.inQueParams.prevLinkId 	= pDec->ipcbit_outhost_Link.link_id;
	pIpcBitsInVideoPrm->baseCreateParams.inQueParams.prevLinkQueId	= 0;
	pIpcBitsInVideoPrm->baseCreateParams.numOutQue					= 1;
	pIpcBitsInVideoPrm->baseCreateParams.outQueParams[0].nextLink	= pDec->decLink.link_id;
	pIpcBitsInVideoPrm->baseCreateParams.noNotifyMode				= TRUE;
	pIpcBitsInVideoPrm->baseCreateParams.notifyNextLink 			= TRUE;
	pIpcBitsInVideoPrm->baseCreateParams.notifyPrevLink 			= FALSE;

	pDecPrm->inQueParams.prevLinkId 	= pDec->ipcbit_invideo_Link.link_id;
	pDecPrm->inQueParams.prevLinkQueId	= 0;
	pDecPrm->outQueParams.nextLink		= pDec->ipc_outvideo_Link.link_id;
	pDecPrm->tilerEnable				= FALSE;

	for(queId = 0; queId < pDec->iDecNum; queId++) {
		pDecPrm->chCreateParams[queId].format		 = IVIDEO_H264HP;
		pDecPrm->chCreateParams[queId].profile		 = IH264VDEC_PROFILE_ANY;
		pDecPrm->chCreateParams[queId].targetMaxWidth  = 1920;
		pDecPrm->chCreateParams[queId].targetMaxHeight = 1080;
		pDecPrm->chCreateParams[queId].fieldMergeDecodeEnable = FALSE;
		pDecPrm->chCreateParams[queId].defaultDynamicParams.targetFrameRate = 30;
		pDecPrm->chCreateParams[queId].defaultDynamicParams.targetBitRate = (10 * 1000 * 1000);
		pDecPrm->chCreateParams[queId].numBufPerCh = 4;
	}

	pIpcOutVideoPrm->inQueParams.prevLinkId 	= pDec->decLink.link_id;
	pIpcOutVideoPrm->inQueParams.prevLinkQueId	= 0;
	pIpcOutVideoPrm->numOutQue				= 1;
	pIpcOutVideoPrm->outQueParams[0].nextLink	= pDec->ipc_invpss_Link.link_id;
	pIpcOutVideoPrm->notifyNextLink 			= TRUE;
	pIpcOutVideoPrm->notifyPrevLink 			= TRUE;
	pIpcOutVideoPrm->noNotifyMode				= FALSE;

	pIpcInVpssPrm->inQueParams.prevLinkId		= pDec->ipc_outvideo_Link.link_id;
	pIpcInVpssPrm->inQueParams.prevLinkQueId	= 0;
	pIpcInVpssPrm->numOutQue				= 1;
	pIpcInVpssPrm->outQueParams[0].nextLink 	= nextLinkId;
	pIpcInVpssPrm->notifyNextLink				= TRUE;
	pIpcInVpssPrm->notifyPrevLink				= TRUE;
	pIpcInVpssPrm->noNotifyMode 				= FALSE;

}


#endif	//__REACH_LINK_H__

