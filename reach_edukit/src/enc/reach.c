
#include "reach.h"
#include "video.h"
Int32 reach_edukit_link_init(EduKitLinkStruct_t *pstruct)
{
	pstruct->start_runing = 1;
	pstruct->decoderLink.iDecNum = DEC_STREAMNUM;
	pstruct->encoderLink.sEncNum.h264 = 4;
	pstruct->encoderLink.sEncNum.jpeg = 0;

#ifdef HAVE_JPEG
	pstruct->encoderLink.sEncNum.jpeg = 1;
#endif
	//sd-card didn't have osd. modify by zm
	pstruct->enableOsdAlgLink = 0;
	pstruct->audioencLink[0].pacaphandle = NULL;
	pstruct->audioencLink[0].paenchandle = NULL;
	pstruct->audioencLink[1].pacaphandle = NULL;
	pstruct->audioencLink[1].paenchandle = NULL;

	pstruct->capLink.link_id = SYSTEM_LINK_ID_CAPTURE;
	pstruct->nullSrcLink.link_id = SYSTEM_VPSS_LINK_ID_NULL_SRC_0;
	pstruct->deiLink.link_id  = SYSTEM_LINK_ID_DEI_HQ_0;
	pstruct->dupLink[0].link_id = SYSTEM_VPSS_LINK_ID_DUP_0;
	pstruct->dupLink[1].link_id = SYSTEM_VPSS_LINK_ID_DUP_1;

	pstruct->mergeLink[0].link_id = SYSTEM_VPSS_LINK_ID_MERGE_0;
	pstruct->mergeLink[1].link_id = SYSTEM_VPSS_LINK_ID_MERGE_1;
	pstruct->mergeLink[2].link_id = SYSTEM_VPSS_LINK_ID_MERGE_2;

	pstruct->selectLink[0].link_id = SYSTEM_VPSS_LINK_ID_SELECT_0;
	pstruct->selectLink[1].link_id = SYSTEM_VPSS_LINK_ID_SELECT_1;
	pstruct->selectLink[2].link_id = SYSTEM_VPSS_LINK_ID_SELECT_2;

	pstruct->nsfLink.link_id = SYSTEM_LINK_ID_NSF_0;
	pstruct->nullLinkId  = SYSTEM_VPSS_LINK_ID_NULL_0;
	pstruct->sclrLink.link_id = SYSTEM_LINK_ID_SCLR_INST_0;
	pstruct->encoderLink.encLink.link_id = SYSTEM_LINK_ID_VENC_0;
	pstruct->decoderLink.decLink.link_id = SYSTEM_LINK_ID_VDEC_0;

	pstruct->disLink.link_id = SYSTEM_LINK_ID_DISPLAY_0;

	pstruct->encoderLink.ipc_outvpss_Link.link_id = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
	pstruct->encoderLink.ipc_invideo_Link.link_id = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;
	pstruct->encoderLink.ipcbit_outvideo_Link.link_id = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
	pstruct->encoderLink.ipcbit_inhost_Link.link_id = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;

	pstruct->decoderLink.ipcbit_outhost_Link.link_id = SYSTEM_HOST_LINK_ID_IPC_BITS_OUT_0;
	pstruct->decoderLink.ipcbit_invideo_Link.link_id = SYSTEM_VIDEO_LINK_ID_IPC_BITS_IN_0;
	pstruct->decoderLink.ipc_outvideo_Link.link_id = SYSTEM_VIDEO_LINK_ID_IPC_OUT_M3_0;
	pstruct->decoderLink.ipc_invpss_Link.link_id = SYSTEM_VPSS_LINK_ID_IPC_IN_M3_0;

	pstruct->ipcFramesOutVpssToDspId[0]   	= SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_1;
	pstruct->ipcFrames_indsp_link[0].link_id = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;
	pstruct->osd_dspAlg_Link[0].link_id   =  SYSTEM_LINK_ID_ALG_0;



	cap_init_create_param(&(pstruct->capLink.create_params));
	nullsrc_init_create_param(&(pstruct->nullSrcLink.create_params));
	dei_init_create_param(&(pstruct->deiLink.create_params));
	dup_init_create_param(&(pstruct->dupLink[0].create_params));
	dup_init_create_param(&(pstruct->dupLink[1].create_params));
	merge_init_create_param(&(pstruct->mergeLink[0].create_params));
	merge_init_create_param(&(pstruct->mergeLink[1].create_params));
	merge_init_create_param(&(pstruct->mergeLink[2].create_params));

	select_init_create_param(&(pstruct->selectLink[0].create_params));
	select_init_create_param(&(pstruct->selectLink[1].create_params));
	select_init_create_param(&(pstruct->selectLink[2].create_params));

	nsf_init_create_param(&(pstruct->nsfLink.create_params));

	enc_init_create_param(&(pstruct->encoderLink.encLink.create_params));
	sclr_init_create_param(&(pstruct->sclrLink.create_params));
	dec_init_create_param(&(pstruct->decoderLink.decLink.create_params));
	dis_init_create_param(&(pstruct->disLink.create_params));

	ipcoutm3_init_create_param(&(pstruct->encoderLink.ipc_outvpss_Link.create_params));
	ipcinm3_init_create_param(&(pstruct->encoderLink.ipc_invideo_Link.create_params));
	REACH_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams, pstruct->encoderLink.ipcbit_outvideo_Link.create_params);
	REACH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams, pstruct->encoderLink.ipcbit_inhost_Link.create_params);

	REACH_INIT_STRUCT(IpcBitsOutLinkHLOS_CreateParams, pstruct->decoderLink.ipcbit_outhost_Link.create_params);
	REACH_INIT_STRUCT(IpcBitsInLinkRTOS_CreateParams, pstruct->decoderLink.ipcbit_invideo_Link.create_params);
	ipcoutm3_init_create_param(&(pstruct->decoderLink.ipc_outvideo_Link.create_params));
	ipcinm3_init_create_param(&(pstruct->decoderLink.ipc_invpss_Link.create_params));

	REACH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams, (pstruct->ipcFramesOutVpssToDspPrm[0]));
	REACH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams  , (pstruct->ipcFrames_indsp_link[0].create_params));
	REACH_INIT_STRUCT(AlgLink_CreateParams				 , (pstruct->osd_dspAlg_Link[0].create_params));


	return 0;
}



Int32 reach_edukit_create_and_start(EduKitLinkStruct_t *pstruct)
{
	System_linkCreate(pstruct->capLink.link_id, &(pstruct->capLink.create_params), sizeof(CaptureLink_CreateParams));
	cap_config_videodecoder(pstruct->capLink.link_id);
	System_linkCreate(pstruct->deiLink.link_id, &(pstruct->deiLink.create_params), sizeof(DeiLink_CreateParams));
	System_linkCreate(pstruct->dupLink[0].link_id, &(pstruct->dupLink[0].create_params), sizeof(DupLink_CreateParams));
	System_linkCreate(pstruct->nullSrcLink.link_id, &(pstruct->nullSrcLink.create_params), sizeof(NullSrcLink_CreateParams));

	System_linkCreate(pstruct->decoderLink.ipcbit_outhost_Link.link_id, &(pstruct->decoderLink.ipcbit_outhost_Link.create_params), sizeof(IpcBitsOutLinkHLOS_CreateParams));
	System_linkCreate(pstruct->decoderLink.ipcbit_invideo_Link.link_id, &(pstruct->decoderLink.ipcbit_invideo_Link.create_params), sizeof(IpcBitsInLinkRTOS_CreateParams));
	System_linkCreate(pstruct->decoderLink.decLink.link_id, &(pstruct->decoderLink.decLink.create_params), sizeof(DecLink_CreateParams));
	System_linkCreate(pstruct->decoderLink.ipc_outvideo_Link.link_id, &(pstruct->decoderLink.ipc_outvideo_Link.create_params), sizeof(IpcLink_CreateParams));
	System_linkCreate(pstruct->decoderLink.ipc_invpss_Link.link_id, &(pstruct->decoderLink.ipc_invpss_Link.create_params), sizeof(IpcLink_CreateParams));

	System_linkCreate(pstruct->mergeLink[0].link_id, &(pstruct->mergeLink[0].create_params), sizeof(MergeLink_CreateParams));
	System_linkCreate(pstruct->selectLink[0].link_id, &(pstruct->selectLink[0].create_params), sizeof(SelectLink_CreateParams));

	System_linkCreate(pstruct->sclrLink.link_id, &(pstruct->sclrLink.create_params), sizeof(SclrLink_CreateParams));
	System_linkCreate(pstruct->selectLink[1].link_id, &(pstruct->selectLink[1].create_params), sizeof(SelectLink_CreateParams));
	System_linkCreate(pstruct->disLink.link_id, &(pstruct->disLink.create_params), sizeof(DisplayLink_CreateParams));
	System_linkCreate(pstruct->nsfLink.link_id, &(pstruct->nsfLink.create_params), sizeof(NsfLink_CreateParams));

	System_linkCreate(pstruct->selectLink[2].link_id, &(pstruct->selectLink[2].create_params), sizeof(SelectLink_CreateParams));

	System_linkCreate(pstruct->ipcFramesOutVpssToDspId[0], &(pstruct->ipcFramesOutVpssToDspPrm[0]), sizeof(IpcFramesOutLinkRTOS_CreateParams));
	System_linkCreate(pstruct->ipcFrames_indsp_link[0].link_id, &(pstruct->ipcFrames_indsp_link[0].create_params), sizeof(IpcFramesInLinkRTOS_CreateParams));
	System_linkCreate(pstruct->osd_dspAlg_Link[0].link_id, &(pstruct->osd_dspAlg_Link[0].create_params), sizeof(AlgLink_CreateParams));

	System_linkCreate(pstruct->dupLink[1].link_id, &(pstruct->dupLink[1].create_params), sizeof(DupLink_CreateParams));

	System_linkCreate(pstruct->mergeLink[1].link_id, &(pstruct->mergeLink[1].create_params), sizeof(MergeLink_CreateParams));

	System_linkCreate(pstruct->encoderLink.ipc_outvpss_Link.link_id, &(pstruct->encoderLink.ipc_outvpss_Link.create_params), sizeof(IpcLink_CreateParams));
	System_linkCreate(pstruct->encoderLink.ipc_invideo_Link.link_id, &(pstruct->encoderLink.ipc_invideo_Link.create_params), sizeof(IpcLink_CreateParams));
	System_linkCreate(pstruct->encoderLink.encLink.link_id, &(pstruct->encoderLink.encLink.create_params), sizeof(EncLink_CreateParams));
	System_linkCreate(pstruct->encoderLink.ipcbit_outvideo_Link.link_id, &(pstruct->encoderLink.ipcbit_outvideo_Link.create_params), sizeof(IpcBitsOutLinkRTOS_CreateParams));
	System_linkCreate(pstruct->encoderLink.ipcbit_inhost_Link.link_id, &(pstruct->encoderLink.ipcbit_inhost_Link.create_params), sizeof(IpcBitsInLinkHLOS_CreateParams));

	System_linkStart(pstruct->encoderLink.ipcbit_inhost_Link.link_id);
	System_linkStart(pstruct->encoderLink.ipcbit_outvideo_Link.link_id);
	System_linkStart(pstruct->encoderLink.encLink.link_id);
	System_linkStart(pstruct->encoderLink.ipc_invideo_Link.link_id);
	System_linkStart(pstruct->encoderLink.ipc_outvpss_Link.link_id);

	System_linkStart(pstruct->mergeLink[1].link_id);

	System_linkStart(pstruct->selectLink[2].link_id);

	System_linkStart(pstruct->nsfLink.link_id);
	System_linkStart(pstruct->disLink.link_id);
	System_linkStart(pstruct->selectLink[1].link_id);
	System_linkStart(pstruct->sclrLink.link_id);

	System_linkStart(pstruct->selectLink[0].link_id);
	System_linkStart(pstruct->mergeLink[0].link_id);

	System_linkStart(pstruct->decoderLink.ipc_invpss_Link.link_id);
	System_linkStart(pstruct->decoderLink.ipc_outvideo_Link.link_id);
	System_linkStart(pstruct->decoderLink.decLink.link_id);
	System_linkStart(pstruct->decoderLink.ipcbit_invideo_Link.link_id);
	System_linkStart(pstruct->decoderLink.ipcbit_outhost_Link.link_id);

	System_linkStart(pstruct->nullSrcLink.link_id);
	System_linkStart(pstruct->deiLink.link_id);
	System_linkStart(pstruct->capLink.link_id);

	return 0;
}

Int32 reach_edukit_link_process(EduKitLinkStruct_t *pstruct)
{
	printf("[reach_edukit_link_process] pstruct:[%p]\n", pstruct);
	MergeLink_CreateParams			*pMergePrm1;

	SclrLink_CreateParams			*pSclrPrm;
	IpcLink_CreateParams			*pIpcOutVpssPrm;
	//Capture===============================================
	Int32 nextLinkId[2] = {
		pstruct->deiLink.link_id,
		pstruct->mergeLink[0].link_id
	};
	UInt32 vipInstId[2] = {
		SYSTEM_CAPTURE_INST_VIP0_PORTA, //SD
		SYSTEM_CAPTURE_INST_VIP1_PORTA //VGA
	};
	setVpssCaptureParams(&(pstruct->capLink.create_params),
	                     2, vipInstId, nextLinkId);
	//Dei===================================================
	setVpssDeiParams(&(pstruct->deiLink.create_params),
	                 pstruct->capLink.link_id, 0,
	                 pstruct->dupLink[0].link_id);
	//Dup0==================================================
	UInt32 DupNextId0[2] = {
		pstruct->mergeLink[0].link_id,
		pstruct->mergeLink[0].link_id
	};
	setVpssDupParams(&pstruct->dupLink[0].create_params,
	                 pstruct->deiLink.link_id, 1, 2, DupNextId0);
	//NullSrc=================================================
	setVpssNullSrcParams(&(pstruct->nullSrcLink.create_params),
	                     1, pstruct->mergeLink[0].link_id);
	//Dec===================================================
	setVideoDecoderParams(&(pstruct->decoderLink), pstruct->mergeLink[0].link_id);
	//Merge0================================================
	Int32 prevLinkId0[5] = {
		pstruct->dupLink[0].link_id,
		pstruct->dupLink[0].link_id,
		pstruct->capLink.link_id,
		pstruct->nullSrcLink.link_id,
		pstruct->decoderLink.ipc_invpss_Link.link_id
	};
	Int32 prevLinkQueId0[5] = {0, 1, 1, 0, 0};
	setVpssMergeParams(&(pstruct->mergeLink[0].create_params),
	                   5, prevLinkId0, prevLinkQueId0,
	                   pstruct->selectLink[0].link_id);
	//Select0================================================
	Int32 nextLinkId0[1] = {
		pstruct->sclrLink.link_id
	};
	Int32 numOutCh0[1] = {5};
	char inChNum0[5] = {0, 1, 3, 4, 5};
	setVpssSelectParams(&(pstruct->selectLink[0].create_params),
	                    pstruct->mergeLink[0].link_id, 0,
	                    1, nextLinkId0,
	                    numOutCh0, inChNum0);
	//Sclr===================================================
	setVpssSclrParams(&(pstruct->sclrLink.create_params),
	                  pstruct->selectLink[0].link_id, 0,
	                  pstruct->selectLink[1].link_id,
	                  SCLR_SCALE_MODE_ABSOLUTE, 1920, 1080);
	//Select1=================================================
	Int32 nextLinkId1[2] = {
		pstruct->nsfLink.link_id,
		pstruct->disLink.link_id
	};
	Int32 numOutCh1[2] = {4, 1};
	char inChNum1[2][4] = {{0, 1, 2, 3}, {4}};
	setVpssSelectParams(&(pstruct->selectLink[1].create_params),
	                    pstruct->sclrLink.link_id, 0,
	                    2, nextLinkId1,
	                    numOutCh1, inChNum1);
	//HDMIDisplay==============================================
	setHDMIDisplayParams(&(pstruct->disLink.create_params),
	                     pstruct->selectLink[1].link_id, 1,
	                     pstruct->HDMIRes.ResIdx);

	//Nsf====================================================
	setVpssNsfParams(&(pstruct->nsfLink.create_params),
	                 pstruct->selectLink[1].link_id, 0,
	                 pstruct->selectLink[2].link_id);
	//Select2=================================================
	Int32 nextLinkId2[2] = {
		pstruct->ipcFramesOutVpssToDspId[0],
		pstruct->dupLink[1].link_id
	};
	Int32 numOutCh2[2] = {3, 1};
	char inChNum2[2][3] = {{0, 1 , 3}, {2}};
	setVpssSelectParams(&(pstruct->selectLink[2].create_params),
	                    pstruct->nsfLink.link_id, 0,
	                    2, nextLinkId2,
	                    numOutCh2, inChNum2);
	//OutVpssToDsp=================================================
	setIpcFramesOutVpssToDspParams(&(pstruct->ipcFramesOutVpssToDspPrm[0]),
	                               pstruct->selectLink[2].link_id, 0,
	                               pstruct->mergeLink[1].link_id,
	                               pstruct->ipcFrames_indsp_link[0].link_id);
	//InDsp=======================================================
	setIpcFramesInDspParams(&(pstruct->ipcFrames_indsp_link[0].create_params),
	                        pstruct->ipcFramesOutVpssToDspId[0], 0,
	                        pstruct->osd_dspAlg_Link[0].link_id);
	//Alg=========================================================
	setOsdDspTrackAlgparams(&(pstruct->osd_dspAlg_Link[0].create_params),
	                        pstruct->ipcFrames_indsp_link[0].link_id, 0);
	//Dup1==================================================
	UInt32 DupNextId1[2] = {
		pstruct->mergeLink[1].link_id,
		pstruct->mergeLink[1].link_id
	};
	setVpssDupParams(&pstruct->dupLink[1].create_params, pstruct->selectLink[2].link_id, 1, 2, DupNextId1);
	//Merge1======================================================
	Int32 prevLinkId1[3] = {
		pstruct->ipcFramesOutVpssToDspId[0],
		pstruct->dupLink[1].link_id,
		pstruct->dupLink[1].link_id
	};
	Int32 prevLinkQueId1[3] = {0, 0, 1};
	setVpssMergeParams(&(pstruct->mergeLink[1].create_params),
	                   3, prevLinkId1, prevLinkQueId1,
	                   pstruct->encoderLink.ipc_outvpss_Link.link_id);
	//Enc===================================================
	setVideoEncoderParams(&(pstruct->encoderLink),
	                      pstruct->mergeLink[1].link_id, 0);


}

