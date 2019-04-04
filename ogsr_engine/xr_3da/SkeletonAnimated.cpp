//---------------------------------------------------------------------------
#include 	"stdafx.h"


#include 	"SkeletonAnimated.h"
#include	"SkeletonX.h"
#include	"fmesh.h"

extern int	psSkeletonUpdate;

//////////////////////////////////////////////////////////////////////////
// BoneInstance methods
void		CBlendInstance::construct()
{	
	ZeroMemory			(this,sizeof(*this));
}
void		CBlendInstance::blend_add	(CBlend* H)
{	
	if ( Blend.size() == MAX_BLENDED )	{
		if(H->fall_at_end)
						return;
		BlendSVecIt _d	= Blend.begin();
		for (BlendSVecIt it=Blend.begin()+1; it!=Blend.end(); it++)
			if ((*it)->blendAmount<(*_d)->blendAmount) _d=it;
		Blend.erase		(_d);
	}
	VERIFY (Blend.size()<MAX_BLENDED);
	Blend.push_back(H);
}
void		CBlendInstance::blend_remove	(CBlend* H)
{
	CBlend** I = std::find(Blend.begin(),Blend.end(),H);
	if (I!=Blend.end())	Blend.erase(I);
}

// Motion control
void	CKinematicsAnimated::Bone_Motion_Start		(CBoneData* bd, CBlend* handle) 
{
	LL_GetBlendInstance	(bd->GetSelfID()).blend_add	(handle);
	for (vecBonesIt I=bd->children.begin(); I!=bd->children.end(); I++)                        
		Bone_Motion_Start	(*I,handle);
}
void	CKinematicsAnimated::Bone_Motion_Stop		(CBoneData* bd, CBlend* handle) 
{
	LL_GetBlendInstance	(bd->GetSelfID()).blend_remove	(handle);
	for (vecBonesIt I=bd->children.begin(); I!=bd->children.end(); I++)
		Bone_Motion_Stop	(*I,handle);
}
void	CKinematicsAnimated::Bone_Motion_Start_IM	(CBoneData* bd,  CBlend* handle) 
{
	LL_GetBlendInstance	(bd->GetSelfID()).blend_add		(handle);
}
void	CKinematicsAnimated::Bone_Motion_Stop_IM	(CBoneData* bd, CBlend* handle) 
{
	LL_GetBlendInstance	(bd->GetSelfID()).blend_remove	(handle);
}

#ifdef DEBUG
std::pair<LPCSTR,LPCSTR> CKinematicsAnimated::LL_MotionDefName_dbg	(MotionID ID)
{
	shared_motions& s_mots	= m_Motions[ID.slot].motions;
	accel_map::iterator _I, _E=s_mots.motion_map()->end();
	for (_I	= s_mots.motion_map()->begin(); _I!=_E; ++_I)	if (_I->second==ID.idx) return std::make_pair(*_I->first,*s_mots.id());
	return std::make_pair((LPCSTR)0,(LPCSTR)0);
}

u32	CKinematicsAnimated::LL_PartBlendsCount	( u32 bone_part_id )
{
	return blend_cycle(bone_part_id).size();
}

CBlend*	CKinematicsAnimated::LL_PartBlend	( u32 bone_part_id, u32 n  )
{
	if( LL_PartBlendsCount(bone_part_id)<=n )
		return 0;
	return blend_cycle(bone_part_id)[ n ];
}
/*
LPCSTR CKinematicsAnimated::LL_MotionDefName_dbg	(LPVOID ptr)
{
//.
	// cycles
	mdef::const_iterator I,E;
	I = motions.cycle()->begin(); 
	E = motions.cycle()->end(); 
	for ( ; I != E; ++I) if (&(*I).second == ptr) return *(*I).first;
	// fxs
	I = motions.fx()->begin(); 
	E = motions.fx()->end(); 
	for ( ; I != E; ++I) if (&(*I).second == ptr) return *(*I).first;
	return 0;
}
*/
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
MotionID CKinematicsAnimated::LL_MotionID	(LPCSTR B)
{
	MotionID motion_ID;
	for (int k=int(m_Motions.size())-1; k>=0; --k){
    	shared_motions* s_mots	= &m_Motions[k].motions;
		accel_map::iterator I 	= s_mots->motion_map()->find(LPSTR(B));
    	if (I!=s_mots->motion_map()->end())	{ motion_ID.set(u16(k),I->second); break; }
    }
    return motion_ID;
}
u16 CKinematicsAnimated::LL_PartID		(LPCSTR B)
{
	if (0==m_Partition)	return BI_NONE;
	for (u16 id=0; id<MAX_PARTS; id++) {
		CPartDef&	P = (*m_Partition)[id];
		if (0==P.Name)	continue;
		if (0==stricmp(B,*P.Name)) return id;
	}
	return BI_NONE;
}

// cycles
MotionID CKinematicsAnimated::ID_Cycle_Safe(LPCSTR  N)
{
	MotionID motion_ID;
	for (int k=int(m_Motions.size())-1; k>=0; --k){
    	shared_motions* s_mots			= &m_Motions[k].motions;
		accel_map::const_iterator I 	= s_mots->cycle()->find(LPSTR(N));
		if (I!=s_mots->cycle()->end())	{	motion_ID.set(u16(k),I->second); break;}
    }
    return motion_ID;
}
MotionID CKinematicsAnimated::ID_Cycle	(shared_str  N)
{
	MotionID motion_ID		= ID_Cycle_Safe	(N);	R_ASSERT3(motion_ID.valid(),"! MODEL: can't find cycle: ", N.c_str());
    return motion_ID;
}
MotionID CKinematicsAnimated::ID_Cycle_Safe(shared_str  N)
{
	MotionID motion_ID;
	for (int k=int(m_Motions.size())-1; k>=0; --k){
		shared_motions* s_mots			= &m_Motions[k].motions;
		accel_map::const_iterator I 	= s_mots->cycle()->find(N);
		if (I!=s_mots->cycle()->end())	{	motion_ID.set(u16(k),I->second); break;}
	}
	return motion_ID;
}
MotionID CKinematicsAnimated::ID_Cycle	(LPCSTR  N)
{
	MotionID motion_ID		= ID_Cycle_Safe	(N);	R_ASSERT3(motion_ID.valid(),"! MODEL: can't find cycle: ", N);
	return motion_ID;
}
void	CKinematicsAnimated::LL_FadeCycle(u16 part, float falloff, u8 mask_channel /*= (1<<0)*/)
{
	BlendSVec&	Blend		= blend_cycles[part];
	
	for (u32 I=0; I<Blend.size(); I++)
	{
		CBlend& B			= *Blend[I];
		if(!(mask_channel&(1<<B.channel)))
			continue;
		//B.blend				= CBlend::eFalloff;
		B.set_falloff_state();
		B.blendFalloff		= falloff;
		//B.blendAccrue		= B.timeCurrent;
		if (B.stop_at_end)  B.stop_at_end_callback = FALSE;		// callback не должен приходить!
	}
}
void	CKinematicsAnimated::LL_CloseCycle(u16 part, u8 mask_channel /*= (1<<0)*/)
{
	if (BI_NONE==part)		return;
	if (part>=MAX_PARTS)	return;

	// destroy cycle(s)
	BlendSVecIt	I = blend_cycles[part].begin(), E = blend_cycles[part].end();
	for (; I!=E; I++){
		CBlend& B = *(*I);
		if(!(mask_channel&(1<<B.channel)))
					continue;
		//B.blend = CBlend::eFREE_SLOT;
		B.set_free_state();
		
		CPartDef& P	= (*m_Partition)[B.bone_or_part];
		for (u32 i=0; i<P.bones.size(); i++)
			Bone_Motion_Stop_IM	((*bones)[P.bones[i]],*I);

		blend_cycles[part].erase(I);// ?
		E				= blend_cycles[part].end(); I--; 
	}
	//blend_cycles[part].clear	(); // ?
}

float CKinematicsAnimated::get_animation_length (MotionID motion_ID)
{
	VERIFY							( motion_ID.slot<m_Motions.size() );

	SMotionsSlot& slot			=	m_Motions[motion_ID.slot];

	VERIFY							( LL_GetBoneRoot() < slot.bone_motions.size() );

	MotionVec*	bone_motions	=	slot.bone_motions[LL_GetBoneRoot()];

	VERIFY							(motion_ID.idx < bone_motions->size());

    CMotionDef* const m_def		=	slot.motions.motion_def(motion_ID.idx);
	
	float const	anim_speed		=	m_def ? m_def->Speed() : 1.f;

	return							bone_motions->at(motion_ID.idx).GetLength() / anim_speed;
}

void CKinematicsAnimated::IBlendSetup(CBlend& B,u16 part,u8 channel, MotionID motion_ID, BOOL  bMixing, float blendAccrue, float blendFalloff, float Speed, BOOL noloop, PlayCallback Callback, LPVOID CallbackParam)
{
	VERIFY(B.channel<MAX_CHANNELS);
	// Setup blend params
	if (bMixing)	{
		//B.blend		= CBlend::eAccrue;
		B.set_accrue_state();
		B.blendAmount	= EPS_S;
	} else {
		//B.blend		= CBlend::eFixed;
		//B.blend		= CBlend::eAccrue;
		B.set_accrue_state();
		B.blendAmount	= 1;
	}
	B.blendAccrue	= blendAccrue;
	B.blendFalloff	= 0; // blendFalloff used for previous cycles
	B.blendPower	= 1;
	B.speed			= Speed;
	B.motionID		= motion_ID;
	B.timeCurrent	= 0;
	B.timeTotal		= m_Motions[B.motionID.slot].bone_motions[LL_GetBoneRoot()]->at(motion_ID.idx).GetLength();
	B.bone_or_part	= part;
	B.stop_at_end	= noloop;
	B.playing		= TRUE;
	B.stop_at_end_callback = TRUE;
	B.Callback		= Callback;
	B.CallbackParam = CallbackParam;

	B.channel		= channel;
	B.fall_at_end	= B.stop_at_end && (channel > 1);
}
void CKinematicsAnimated::IFXBlendSetup(CBlend &B, MotionID motion_ID, float blendAccrue, float blendFalloff,float Power ,float Speed,u16 bone)
{
	//B.blend			= CBlend::eAccrue;
	B.set_accrue_state();
	B.blendAmount	= EPS_S;
	B.blendAccrue	= blendAccrue;
	B.blendFalloff	= blendFalloff;
	B.blendPower	= Power;
	B.speed			= Speed;
	B.motionID		= motion_ID;
	B.timeCurrent	= 0;
	B.timeTotal		= m_Motions[B.motionID.slot].bone_motions[bone]->at(motion_ID.idx).GetLength();
	B.bone_or_part	= bone;

	B.playing		= TRUE;
	B.stop_at_end_callback = TRUE;
	B.stop_at_end	= FALSE;
	//
	B.Callback		= 0;
	B.CallbackParam = 0;

	B.channel		= 0;
	B.fall_at_end	= FALSE;	
}
CBlend*	CKinematicsAnimated::LL_PlayCycle(u16 part, MotionID motion_ID, BOOL  bMixing, float blendAccrue, float blendFalloff, float Speed, BOOL noloop, PlayCallback Callback, LPVOID CallbackParam,u8 channel/*=0*/ )
{
	// validate and unroll
	if (!motion_ID.valid())	return 0;
	if (BI_NONE==part)		{
		for (u16 i=0; i<MAX_PARTS; i++)
			LL_PlayCycle(i,motion_ID,bMixing,blendAccrue,blendFalloff,Speed,noloop,Callback,CallbackParam,channel);
		return 0;
	}
	if (part>=MAX_PARTS)	return 0;
	if (0==m_Partition->part(part).Name)	return 0;

//	shared_motions* s_mots	= &m_Motions[motion.slot];
//	CMotionDef* m_def		= s_mots->motion_def(motion.idx);
    
	// Process old cycles and create _new_
	if( channel == 0 )
	{
		_DBG_SINGLE_USE_MARKER;
		if (bMixing)	LL_FadeCycle	(part,blendFalloff,1<<channel);
		else			LL_CloseCycle	(part,1<<channel);
	}
	CPartDef& P	=	(*m_Partition)[part];
	CBlend*	B	=	IBlend_Create	();

	_DBG_SINGLE_USE_MARKER;
	IBlendSetup(*B, part,channel, motion_ID, bMixing, blendAccrue, blendFalloff, Speed, noloop, Callback, CallbackParam );
	for (u32 i=0; i<P.bones.size(); i++)
		Bone_Motion_Start_IM	((*bones)[P.bones[i]],B);
	blend_cycles[part].push_back(B);
	return		B;
}
CBlend*	CKinematicsAnimated::LL_PlayCycle		(u16 part, MotionID motion_ID, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam, u8 channel /*=0*/)
{
	VERIFY					(motion_ID.valid()); 
    CMotionDef* m_def		= m_Motions[motion_ID.slot].motions.motion_def(motion_ID.idx);
    VERIFY					(m_def);
	return LL_PlayCycle		(part,motion_ID,bMixIn, 
    						 m_def->Accrue(),m_def->Falloff(),m_def->Speed(),m_def->StopAtEnd(), 
                             Callback,CallbackParam,channel);
}
CBlend*	CKinematicsAnimated::PlayCycle		(LPCSTR  N, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam,u8 channel  /*= 0*/)
{
	MotionID motion_ID		= ID_Cycle(N);
	if (motion_ID.valid())	return PlayCycle(motion_ID,bMixIn,Callback,CallbackParam,channel);
	else					{ Debug.fatal(DEBUG_INFO,"! MODEL: can't find cycle: %s", N); return 0; }
}
CBlend*	CKinematicsAnimated::PlayCycle		(MotionID motion_ID,  BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam,u8 channel /*= 0*/)
{	
	VERIFY					(motion_ID.valid()); 
    CMotionDef* m_def		= m_Motions[motion_ID.slot].motions.motion_def(motion_ID.idx);
    VERIFY					(m_def);
	return LL_PlayCycle		(m_def->bone_or_part,motion_ID,bMixIn, 
    						 m_def->Accrue(),m_def->Falloff(),m_def->Speed(),m_def->StopAtEnd(), 
                             Callback,CallbackParam,channel);
}

// fx'es
MotionID CKinematicsAnimated::ID_FX_Safe		(LPCSTR  N)
{
	MotionID motion_ID;
	for (int k=int(m_Motions.size())-1; k>=0; --k){
    	shared_motions* s_mots	= &m_Motions[k].motions;
		accel_map::iterator I 	= s_mots->fx()->find(LPSTR(N));
		if (I!=s_mots->fx()->end())	{	motion_ID.set(u16(k),I->second); break;}
    }
    return motion_ID;
}
MotionID CKinematicsAnimated::ID_FX			(LPCSTR  N)
{
	MotionID motion_ID		= ID_FX_Safe(N);R_ASSERT3(motion_ID.valid(),"! MODEL: can't find FX: ", N);
    return motion_ID;
}
CBlend*	CKinematicsAnimated::PlayFX			(MotionID motion_ID, float power_scale)
{
	VERIFY					(motion_ID.valid()); 
    CMotionDef* m_def		= m_Motions[motion_ID.slot].motions.motion_def(motion_ID.idx);
    VERIFY					(m_def);
	return LL_PlayFX		(m_def->bone_or_part,motion_ID,
    						 m_def->Accrue(),m_def->Falloff(),
                             m_def->Speed(),m_def->Power()*power_scale);
}
CBlend*	CKinematicsAnimated::PlayFX			(LPCSTR  N, float power_scale)
{
	MotionID motion_ID		= ID_FX(N);
    return PlayFX 			(motion_ID,power_scale);
}
//u16 part,u8 channel, MotionID motion_ID, BOOL  bMixing, float blendAccrue, float blendFalloff, float Speed, BOOL noloop, PlayCallback Callback, LPVOID CallbackParam)

CBlend*	CKinematicsAnimated::LL_PlayFX		(u16 bone, MotionID motion_ID, float blendAccrue, float blendFalloff, float Speed, float Power)
{
	if (!motion_ID.valid())	return 0;
	if (blend_fx.size()>=MAX_BLENDED) return 0;
	if (BI_NONE==bone)		bone = iRoot;
	
	CBlend*	B		= IBlend_Create();
	_DBG_SINGLE_USE_MARKER;
	IFXBlendSetup(*B,motion_ID,blendAccrue,blendFalloff,Power,Speed,bone);
	Bone_Motion_Start	((*bones)[bone],B);
	
	blend_fx.push_back(B);
	return			B;
}

void	CKinematicsAnimated::DestroyCycle	(CBlend &B)
{
	if( GetBlendDestroyCallback	( ) )
			GetBlendDestroyCallback	( )->BlendDestroy( B );
	//B.blend 		= CBlend::eFREE_SLOT;
	B.set_free_state( );
	const CPartDef& P		= m_Partition->part( B.bone_or_part );
	for ( u32 i=0; i<P.bones.size(); i++ )
		Bone_Motion_Stop_IM( (*bones)[P.bones[i]], &B );
}

IC void UpdateBlendTime(CBlend &B,float dt)
{
	if (B.playing) {
		B.timeCurrent += dt*B.speed; // stop@end - time is not going 
	}
}
//returns true if play time out
IC bool UpdatePlayBlend(CBlend &B,float dt)
{

	B.blendAmount 		+= dt*B.blendAccrue*B.blendPower;

	if (B.blendAmount>B.blendPower)	B.blendAmount = B.blendPower; 

	if (B.stop_at_end && (B.timeCurrent > (B.timeTotal-SAMPLE_SPF/*-EPS*/) )) {
		B.timeCurrent	= B.timeTotal-SAMPLE_SPF /*- EPS*/;		// stop@end - time frozen at the end
		if (B.playing&&B.Callback)	B.Callback(&B);		// callback only once
		B.playing		= FALSE;

		return true;
	}

	return false;
}
IC bool UpdateFalloffBlend(CBlend &B,float dt)
{
	B.blendAmount 		-= dt*B.blendFalloff*B.blendPower;
	return B.blendAmount<=0;
}

void CKinematicsAnimated::LL_UpdateTracks( float dt, bool b_force, bool leave_blends )
{
	BlendSVecIt I,E;
	// Cycles
	for (u16 part=0; part<MAX_PARTS; part++)
	{
		if (0==m_Partition->part(part).Name)
			continue;
		I = blend_cycles[part].begin(); E = blend_cycles[part].end();
		for (; I!=E; I++)
		{
			CBlend& B = *(*I);
			if ( !b_force && B.dwFrame == Device.dwFrame )
					continue;
			B.dwFrame =	Device.dwFrame;
			if( B.update( dt, B.Callback ) && !leave_blends )
			{
				DestroyCycle( B );
				blend_cycles[part].erase( I );
				E = blend_cycles[part].end(); I--; 
			}
			//else{
			//	CMotionDef* m_def						= m_Motions[B.motionID.slot].motions.motion_def(B.motionID.idx);
			//	float timeCurrent						= B.timeCurrent;
			//	xr_vector<motion_marks>::iterator it	= m_def->marks.begin();
			//	xr_vector<motion_marks>::iterator it_e	= m_def->marks.end();
			//	for(;it!=it_e; ++it)
			//	{
			//		if( (*it).pick_mark(timeCurrent) )
			//	}
			//}
		}
	}
	
	LL_UpdateFxTracks(dt);
}
void	CKinematicsAnimated::LL_UpdateFxTracks( float dt )
{
		// FX
	BlendSVecIt I,E;
	I = blend_fx.begin(); E = blend_fx.end();
	for (; I!=E; I++)
	{
		CBlend& B = *(*I);
		if ( !B.stop_at_end_callback )
		{
			B.playing =FALSE;
			continue;
		}
		//B.timeCurrent += dt*B.speed;
		B.update_time( dt );
		switch (B.blend_state()) 
		{
		case CBlend::eFREE_SLOT: 
			NODEFAULT;

		case CBlend::eAccrue:
            B.blendAmount 	+= dt*B.blendAccrue*B.blendPower*B.speed;
			if (B.blendAmount>=B.blendPower) {
				// switch to fixed
				B.blendAmount	= B.blendPower;
				//B.blend			= CBlend::eFalloff;//CBlend::eFixed;
				B.set_falloff_state();
			}
			break;
		case CBlend::eFalloff:
			B.blendAmount 	-= dt*B.blendFalloff*B.blendPower*B.speed;
			if (B.blendAmount<=0) {
				// destroy fx
				//B.blend = CBlend::eFREE_SLOT;
				B.set_free_state();
				Bone_Motion_Stop((*bones)[B.bone_or_part],*I);
				blend_fx.erase(I); 
				E=blend_fx.end(); I--; 
			}
			break;
		default: NODEFAULT;
		}
	}
}
void CKinematicsAnimated::UpdateTracks	()
{
	_DBG_SINGLE_USE_MARKER;
	if (Update_LastTime==Device.dwTimeGlobal) return;
	u32 DT	= Device.dwTimeGlobal-Update_LastTime;
	if (DT>66) DT=66;
	float dt = float(DT)/1000.f;
	
	if( GetUpdateTracksCalback()  )
	{
		if( ( *GetUpdateTracksCalback() )( float(Device.dwTimeGlobal-Update_LastTime)/1000.f, *this ) )
					Update_LastTime = Device.dwTimeGlobal;
		return;
	}
	Update_LastTime 	= Device.dwTimeGlobal;
	LL_UpdateTracks	( dt, false, false );
}

void CKinematicsAnimated::Release()
{
	// xr_free bones
//.	for (u32 i=0; i<bones->size(); i++)
//.	{
//.		CBoneDataAnimated* B	= (CBoneDataAnimated*)(*bones)[i];
//.		B->Motions.clear		();
//.	}

	// destroy shared data
//.	xr_delete(partition);
//.	xr_delete(motion_map);
//.	xr_delete(m_cycle);
//.	xr_delete(m_fx);

    inherited::Release	();
}

CKinematicsAnimated::~CKinematicsAnimated	()
{
	IBoneInstances_Destroy	();
}
CKinematicsAnimated::CKinematicsAnimated(): 
	CKinematics(), 
	blend_instances	( NULL) ,
    m_Partition	( NULL ),
	m_blend_destroy_callback( 0 ),
	m_update_tracks_callback( 0 ),
	Update_LastTime ( 0 )
{
	
}

void	CKinematicsAnimated::IBoneInstances_Create()
{
    inherited::IBoneInstances_Create();
	u32				size	= bones->size();
	blend_instances			= xr_alloc<CBlendInstance>(size);
	for (u32 i=0; i<size; i++)
		blend_instances[i].construct();
}

void	CKinematicsAnimated::IBoneInstances_Destroy()
{
    inherited::IBoneInstances_Destroy();
	if (blend_instances) {
		xr_free(blend_instances);
		blend_instances = NULL;
	}
}

#define PCOPY(a)	a = pFrom->a
void CKinematicsAnimated::Copy(IRender_Visual *P) 
{
	inherited::Copy	(P);

	CKinematicsAnimated* pFrom = (CKinematicsAnimated*)P;
	PCOPY			(m_Motions);
    PCOPY			(m_Partition);

	IBlend_Startup			();
}

void CKinematicsAnimated::Spawn	()
{
	inherited::Spawn		();

	IBlend_Startup			();

	for (u32 i=0; i<bones->size(); i++)
		blend_instances[i].construct();
	m_update_tracks_callback = 0;
	channels.init();

}
void CKinematicsAnimated::ChannelFactorsStartup()
{
	channels.init();
}
void	CKinematicsAnimated::LL_SetChannelFactor (u16	channel,float factor)
{
	channels.set_factor( channel, factor );
}
void CKinematicsAnimated::IBlend_Startup	()
{
	_DBG_SINGLE_USE_MARKER;
	CBlend B; ZeroMemory(&B,sizeof(B));
	//B.blend				= CBlend::eFREE_SLOT;

	B.set_free_state();

#ifdef	DEBUG
	B.set_falloff_state();
#endif

	blend_pool.clear	();
	for (u32 i=0; i<MAX_BLENDED_POOL; i++)
	{
		blend_pool.push_back(B);
#ifdef	DEBUG
		blend_pool.back().set_free_state();
#endif
	}
	// cycles+fx clear
	for (i=0; i<MAX_PARTS; i++)
		blend_cycles[i].clear();
	blend_fx.clear		();
	ChannelFactorsStartup();
}

CBlend*	CKinematicsAnimated::IBlend_Create	()
{
	UpdateTracks	();
	_DBG_SINGLE_USE_MARKER;
	CBlend *I=blend_pool.begin(), *E=blend_pool.end();
	for (; I!=E; I++)
		if (I->blend_state() == CBlend::eFREE_SLOT) return I;
	FATAL("Too many blended motions requisted");
	return 0;
}
void CKinematicsAnimated::Load(const char* N, IReader *data, u32 dwFlags)
{
	inherited::Load	(N, data, dwFlags);

	// Globals
	blend_instances		= NULL;
    m_Partition			= NULL;
	Update_LastTime 	= 0;

	// Load animation
    if (data->find_chunk(OGF_S_MOTION_REFS))
    {
    	string_path		items_nm;
        data->r_stringZ	(items_nm,sizeof(items_nm));
        u32 set_cnt		= _GetItemCount(items_nm);
        R_ASSERT		(set_cnt<MAX_ANIM_SLOT);
		m_Motions.reserve(set_cnt);
    	string_path		nm;
        for (u32 k=0; k<set_cnt; ++k)
        {
        	_GetItem	(items_nm,k,nm);
            xr_strcat		(nm,".omf");
            string_path	fn;
            if (!FS.exist(fn, "$level$", nm))
            {
                if (!FS.exist(fn, "$game_meshes$", nm))
                {
#ifdef _EDITOR
                    Msg			("!Can't find motion file '%s'.",nm);
                    return;
#else
                    Debug.fatal	(DEBUG_INFO,"Can't find motion file '%s'.",nm);
#endif
                }
            }
            // Check compatibility
            m_Motions.push_back				(SMotionsSlot());
            bool create_res = true;
            if( !g_pMotionsContainer->has(nm) ) //optimize fs operations
			{
				IReader* MS						= FS.r_open(fn);
				create_res = m_Motions.back().motions.create	(nm,MS,bones);
				FS.r_close						(MS);
			}
            if(create_res)
				m_Motions.back().motions.create	(nm,NULL,bones);
            else{
            	m_Motions.pop_back	();
                Msg					("! error in model [%s]. Unable to load motion file '%s'.", N, nm);
                }
    	}
    }else
    if (data->find_chunk(OGF_S_MOTION_REFS2))
    {
		u32 set_cnt		= data->r_u32();
		m_Motions.reserve(set_cnt);
    	string_path		nm;
        for (u32 k=0; k<set_cnt; ++k)
        {
			data->r_stringZ	(nm,sizeof(nm));
            xr_strcat			(nm,".omf");
            string_path	fn;
            if (!FS.exist(fn, "$level$", nm))
            {
                if (!FS.exist(fn, "$game_meshes$", nm))
                {
#ifdef _EDITOR
                    Msg			("!Can't find motion file '%s'.",nm);
                    return;
#else
                    Debug.fatal	(DEBUG_INFO,"Can't find motion file '%s'.",nm);
#endif
                }
            }
            // Check compatibility
            m_Motions.push_back				(SMotionsSlot());
            bool create_res = true;
            if( !g_pMotionsContainer->has(nm) ) //optimize fs operations
			{
				IReader* MS						= FS.r_open(fn);
				create_res = m_Motions.back().motions.create	(nm,MS,bones);
				FS.r_close						(MS);
			}
            if(create_res)
				m_Motions.back().motions.create	(nm,NULL,bones);
            else{
            	m_Motions.pop_back	();
                Msg					("! error in model [%s]. Unable to load motion file '%s'.", N, nm);
                }
    	}
    }else    
	{
		string_path	nm;
		strconcat			(sizeof(nm),nm,N,".ogf");
		m_Motions.push_back(SMotionsSlot());
		m_Motions.back().motions.create(nm,data,bones);
    }

    R_ASSERT				(m_Motions.size());

    m_Partition				= m_Motions[0].motions.partition();
	//m_Partition->load		(this,N);
    
	// initialize motions
	for (MotionsSlotVecIt m_it=m_Motions.begin(); m_it!=m_Motions.end(); m_it++){
		SMotionsSlot& MS	= *m_it;
		MS.bone_motions.resize(bones->size());
		for (u32 i=0; i<bones->size(); i++){
			CBoneData* BD		= (*bones)[i];
			MS.bone_motions[i]	= MS.motions.bone_motions(BD->name);
		}
	}

	// Init blend pool
	IBlend_Startup	();

//.	if (motions.cycle()->size()<2)			
//.		Msg("* WARNING: model '%s' has only one motion. Candidate for SkeletonRigid???",N);
}

//------------------------------------------------------------------------------
// calculate
//------------------------------------------------------------------------------
IC void	KEY_Interp	(CKey& D,const CKey& K1,const CKey& K2, float delta)
{
	VERIFY			(_valid(delta));
	VERIFY			(delta>=0.f && delta<=1.f);
	D.Q.slerp		(K1.Q,K2.Q,delta);
	D.T.lerp		(K1.T,K2.T,delta);
}
struct ConsistantKey
{
	const CKey*	K;
	float	w;

	IC void	set(const CKey* _K, float _w)
	{	K = _K; w = _w; }
};

IC bool operator < (const ConsistantKey& A, const ConsistantKey& B)	// note: inverse operator
{	return A.w>B.w; }
/*
IC void MakeKeysConsistant(ConsistantKey *keys, int count)
{
	// sort in decreasing order
	std::sort(keys,keys+count);
	// recalc
	for (int i=0; i<count-1; i++) {
		Fquaternion Q1,Q2;
		Q1.add(keys[i].K->Q,keys[i+1].K->Q);
		Q2.sub(keys[i].K->Q,keys[i+1].K->Q);
		if (Q1.magnitude()<Q2.magnitude())	keys[i+1].K->Q.inverse_with_w();
	}
}
*/
IC void MakeKeysSelected(ConsistantKey *keys, int count)
{
	// sort in decreasing order
	std::sort(keys,keys+count);
}

/*
ICF float smooth(float x)
{
    float x0	= x*2.f-1.f;
    float s 	= (x0<0.f)?-1.f:1.f;
    return ((s*pow(_abs(x0),1.f/1.5f))+1.f)/2.f;
}
*/
IC	void QR2Quat(const CKeyQR &K,Fquaternion &Q)
{
	Q.x		= float(K.x)*KEY_QuantI;
	Q.y		= float(K.y)*KEY_QuantI;
	Q.z		= float(K.z)*KEY_QuantI;
	Q.w		= float(K.w)*KEY_QuantI;
}

IC void QT8_2T(const CKeyQT8& K, const CMotion& M, Fvector &T)
{
	T.x		= float(K.x1)*M._sizeT.x+M._initT.x;
	T.y		= float(K.y1)*M._sizeT.y+M._initT.y;
	T.z		= float(K.z1)*M._sizeT.z+M._initT.z;
}
                            
IC void QT16_2T(const CKeyQT16& K, const CMotion& M, Fvector &T)
{
	T.x		= float(K.x1)*M._sizeT.x+M._initT.x;
	T.y		= float(K.y1)*M._sizeT.y+M._initT.y;
	T.z		= float(K.z1)*M._sizeT.z+M._initT.z;
}                             

IC void Dequantize(CKey& K,const CBlend& BD,const CMotion& M)
{
	CKey*			D		=	&K;
	const CBlend*	B		=	&BD;
	float			time	=	B->timeCurrent*float(SAMPLE_FPS);
	VERIFY			(time >= 0.f);
	u32				frame	=	iFloor(time);
	float			delta	=	time-float(frame);
	u32				count	=	M.get_count();
	// rotation
	if (M.test_flag(flRKeyAbsent)){
		const CKeyQR *		K		=	&M._keysR[0];
		QR2Quat(*K,D->Q);
	}else{
		const CKeyQR*		K1r		=	&M._keysR[(frame+0)%count];
		const CKeyQR*		K2r		=	&M._keysR[(frame+1)%count];
		Fquaternion	Q1,Q2;
		QR2Quat(*K1r,Q1);
		QR2Quat(*K2r,Q2);
		D->Q.slerp	(Q1,Q2,clampr(delta,0.f,1.f));
	}

	// translate
	if (M.test_flag(flTKeyPresent))
	{
       Fvector T1,T2;
       if(M.test_flag(flTKey16IsBit))
       {
            const CKeyQT16*	K1t	= &M._keysT16[(frame+0)%count];
            const CKeyQT16*	K2t	= &M._keysT16[(frame+1)%count];
        

            QT16_2T(*K1t,M,T1);
            QT16_2T(*K2t,M,T2);
        }else
        {
            const CKeyQT8*	K1t	= &M._keysT8[(frame+0)%count];
            const CKeyQT8*	K2t	= &M._keysT8[(frame+1)%count];
        
            QT8_2T(*K1t,M,T1);
            QT8_2T(*K2t,M,T2);
        }
		/*
		T1.x		= float(K1t->x)*M._sizeT.x+M._initT.x;
		T1.y		= float(K1t->y)*M._sizeT.y+M._initT.y;
		T1.z		= float(K1t->z)*M._sizeT.z+M._initT.z;
		T2.x		= float(K2t->x)*M._sizeT.x+M._initT.x;
		T2.y		= float(K2t->y)*M._sizeT.y+M._initT.y;
		T2.z		= float(K2t->z)*M._sizeT.z+M._initT.z;
		*/
		D->T.lerp	(T1,T2,delta);
		/*					
		if ((_abs(D->T.y)>10000) || (_abs(D->T.x)>10000) || (_abs(D->T.z)>10000))
		{
		Log("xxx");
		Log("Blend--------");
		Log("blendAmount", B->blendAmount);
		Log("timeCurrent", B->timeCurrent);
		Log("timeTotal", B->timeTotal);
		Log("bone_or_part", B->bone_or_part);
		Log("blendAccrue", B->blendAccrue);
		Log("blendFalloff", B->blendFalloff);
		Log("blendPower", B->blendPower);
		Log("speed", B->speed);
		Log("playing", B->playing);
		Log("stop_at_end", B->stop_at_end);
		Log("motionID", (u32)B->motionID.idx);
		Log("blend", B->blend);
		Log("dwFrame", B->dwFrame);
		Log("Device.dwFrame", Device.dwFrame);
		Log("Blend-------end");
		Log("Bone",LL_BoneName_dbg(SelfID));
		Log("parent",*parent);
		Msg("K1t %d,%d,%d",K1t->x,K1t->y,K1t->z);
		Msg("K2t %d,%d,%d",K2t->x,K2t->y,K2t->z);
		Log("count",count);
		Log("time",time);
		Log("frame",frame);
		Log("T1",T1);
		Log("T2",T2);
		Log("delta",delta);
		Log("Dt",D->T);
		VERIFY(0);
		}
		*/
	} //if (M.test_flag(flTKeyPresent))
	else
	{
		D->T.set	(M._initT);
	}
}




IC void MixInterlerp( CKey &Result, const CKey	*R, const CBlend* const BA[MAX_BLENDED], int b_count )
{

	VERIFY( MAX_BLENDED >= b_count );
	switch (b_count)
	{
	case 0:
		Result.Q.set	(0,0,0,0);
		Result.T.set	(0,0,0);
		break;
	case 1: 
		Result			= R[0];
		/*
		if(Result.T.y>10000){
		Log("1");
		Log("BLEND_INST",BLEND_INST.Blend.size());
		Log("Bone",LL_BoneName_dbg(SelfID));
		Msg("Result.Q %f,%f,%f,%f",Result.Q.x,Result.Q.y,Result.Q.z,Result.Q.w);
		Log("Result.T",Result.T);
		VERIFY(0);
		}
		*/
		break;
	case 2:
		{
			float w0 = BA[0]->blendAmount;
			float w1 = BA[1]->blendAmount;
			float ws = w0+w1;
			float w;
			if (fis_zero(ws))	w = 0;
			else				w = w1/ws;
#ifdef DEBUG
			//.					if (fis_zero(w0+w1) || (!_valid(w))){
			//.						Debug.fatal		(DEBUG_INFO,"TO ALEXMX VERY IMPORTANT: (TOTAL: %f) w: %f, w0: %f, w1: %f, ws:%f, BIS: %d",w0+w1,w,w0,w1,ws,BLEND_INST.Blend.size());
			//.					}
#endif
			KEY_Interp	(Result,R[0],R[1], clampr(w,0.f,1.f));
			/*
			if(Result.T.y>10000){
			Log("2");
			Log("BLEND_INST",BLEND_INST.Blend.size());
			Log("Bone",LL_BoneName_dbg(SelfID));
			Msg("Result.Q %f,%f,%f,%f",Result.Q.x,Result.Q.y,Result.Q.z,Result.Q.w);
			Log("Result.T",Result.T);
			Log("parent",*parent);
			VERIFY(0);
			}
			*/
		}
		break;
	default:
		{
			//int 	count 	= Blend.size();
			float   total 	= 0;
			ConsistantKey		S[MAX_BLENDED];
			for (int i=0; i<b_count; i++)					
				S[i].set	( R+i, BA[i]->blendAmount );

			std::sort	(S,S+b_count);
			CKey		tmp;
			total		= S[0].w;
			tmp			= *S[0].K;
			for 		(int cnt=1; cnt<b_count; cnt++){
				total	+= S[cnt].w;
				float	d;
				if (fis_zero(total))	d = 0.0f;
				else d	= S[cnt].w/total;

				clampr(d,0.f,1.f);

#ifdef DEBUG
				//.						if ((total==0) || (!_valid(S[cnt].w/total))){
				//.							Debug.fatal		(DEBUG_INFO,"TO ALEXMX VERY IMPORTANT: (TOTAL: %f) w: %f, total: %f, count: %d, real count: %d",total,S[cnt].w,total,count,BLEND_INST.Blend.size());
				//.						}
#endif

				KEY_Interp	(Result,tmp, *S[cnt].K, d );
				tmp 		= Result;
			}
		}
		break;
	}
}



IC void key_sub(CKey &rk, const CKey &k0, const CKey& k1)//sub right
{
	Fquaternion q;
	q.inverse(k1.Q);
	rk.Q.mul(k0.Q,q);
	//rk.Q.normalize();//?
	rk.T.sub(k0.T,k1.T);
}

IC void key_identity(CKey &k)
{
	k.Q.identity();
	k.T.set(0,0,0);
}
IC void key_add(CKey &res, const CKey &k0, const CKey &k1)//add right
{
	
	res.Q.set(Fquaternion().mul(k0.Q,k1.Q));
	//res.Q.normalize();
	res.T.add(k0.T,k1.T);
}
IC void q_scale(Fquaternion &q, float v)
{
	float angl;Fvector ax;
	q.get_axis_angle(ax,angl);
	q.rotation(ax,angl*v);
	//q.normalize();
}
IC void key_scale(CKey &res, const CKey &k, float v)
{
	res = k;
	q_scale(res.Q,v);
	res.T.mul(v);
}
IC void key_mad(CKey &res, const CKey &k0, const CKey& k1, float v)
{
	CKey k ;
	key_scale(k,k1,v);
	key_add(res,k,k0);
}



IC void keys_substruct(CKey	*R, const CKey	*BR, int b_count )
{
	for (int i=0; i<b_count; i++)
	{
		CKey r;
		key_sub (r,R[i],BR[i]);
		R[i] = r;
	}
}



IC void q_scalem(Fmatrix &m, float v)
{
	Fquaternion q;
	q.set(m);
	q_scale(q,v);
	m.rotation(q);
}



//sclale base' * q by scale_factor returns result in matrix  m_res
IC void q_scale_vs_basem(Fmatrix &m_res,const Fquaternion &q, const Fquaternion &base,float scale_factor)
{
	Fmatrix mb,imb;
	mb.rotation(base);
	imb.invert(mb);

	Fmatrix m;m.rotation(q);
	m_res.mul(imb,m);
	q_scalem(m_res,scale_factor);
}


IC void q_add_scaled_basem( Fquaternion &q, const Fquaternion &base, const Fquaternion &q0, const Fquaternion &q1, float v1 )
{
	//VERIFY(0.f =< v && 1.f >= v );
	Fmatrix m0;m0.rotation(q0);
	Fmatrix m,ml1;
	q_scale_vs_basem( ml1, q1, base, v1 );
	m.mul(m0,ml1);
	q.set(m);
	q.normalize();
}

IC float DET(const Fmatrix &a){
	return
		(( a._11 * ( a._22 * a._33 - a._23 * a._32 ) -
		a._12 * ( a._21 * a._33 - a._23 * a._31 ) +
		a._13 * ( a._21 * a._32 - a._22 * a._31 ) ));
}

IC bool check_scale(const Fmatrix &m)
{
	float det = DET(m);
	return (0.8f<det&&det<1.3f);
}

IC bool check_scale(const Fquaternion &q)
{
	Fmatrix m;
	m.rotation(q);
	return check_scale(m);
}

IC void MixFactors(float *F,int b_count)
{
	float sum = 0;
	for (int i=0; i<b_count; i++)
		sum+=F[i];
	for (int i2=0; i2<b_count; i2++)
		F[i2]/=sum;
}
IC void MixinAdd(CKey &Result,const CKey	*R,const float* BA,int b_count)
{
	for (int i=0; i<b_count; i++)
		key_mad(Result,Result,R[i],BA[i]);
}
IC void MixAdd(CKey &Result,const CKey	*R,const float* BA,int b_count)
{
	key_identity(Result);
	MixinAdd(Result,R,BA,b_count);
}
IC void process_single_channel( CKey &Result,const animation::channel_def &ch, const CKey *R, const CBlend* const BA[MAX_BLENDED], int b_count )
{

	MixInterlerp( Result, R, BA, b_count );
	VERIFY( _valid( Result.T ) );
	VERIFY( _valid( Result.Q ) );
}
IC void MixChannels(CKey &Result,const CKey	*R,const animation::channel_def* BA,int b_count)
{
	VERIFY(b_count>0);
	Result = R[0];

	//MixinAdd(Result,R,BA,b_count);
	float	lerp_factor_sum = 0.f;
	for ( int i=1; i<b_count; i++ )
		switch( BA[i].rule.extern_ )
	{
		case animation::add:	
			key_mad( Result, CKey( Result ), R[i], BA[i].factor ); break;

		case animation::lerp:
			lerp_factor_sum +=BA[i].factor;
			KEY_Interp	(Result,CKey( Result ),R[i], BA[i].factor/lerp_factor_sum ); break;
		default: NODEFAULT;
	}
	VERIFY( _valid( Result.T ) );
	VERIFY( _valid( Result.Q ) );
}

void	CKinematicsAnimated::LL_BuldBoneMatrixDequatize( const CBoneData* bd, u8 channel_mask, SKeyTable	&keys )
{
		u16							SelfID				= bd->GetSelfID();
		CBlendInstance				&BLEND_INST			= LL_GetBlendInstance(SelfID);
const	CBlendInstance::BlendSVec	&Blend				= BLEND_INST.blend_vector();
		CKey						BK[MAX_CHANNELS][MAX_BLENDED];	//base keys
		BlendSVecCIt				BI;
		for (BI=Blend.begin(); BI!=Blend.end(); BI++)
		{
			CBlend*			B		 =	*BI;
			int				&b_count =	keys.chanel_blend_conts[B->channel];
			CKey*			D		 =	&keys.keys[B->channel][b_count];
			if(!(channel_mask&(1<<B->channel)))
				continue;
			u8	channel								=  B->channel;
			//keys.blend_factors[channel][b_count]	=  B->blendAmount;
			keys.blends[channel][b_count]			=  B;
			CMotion			&M						= *LL_GetMotion(B->motionID,SelfID);
			Dequantize(*D,*B,M);
			QR2Quat( M._keysR[0], BK[channel][b_count].Q	);
			if(M.test_flag(flTKeyPresent))
            {
            	if(M.test_flag(flTKey16IsBit))
					QT16_2T(M._keysT16[0] ,M ,BK[channel][b_count].T );
                else
					QT8_2T(M._keysT8[0] ,M ,BK[channel][b_count].T );
			}else
				BK[channel][b_count].T.set(M._initT);
			++b_count;
		}
		for(u16 j= 0;MAX_CHANNELS>j;++j)
			if( channels.rule(j).extern_ == animation::add )
				keys_substruct( keys.keys[j], BK[j], keys.chanel_blend_conts[j] );
}

// calculate single bone with key blending 
void	CKinematicsAnimated::LL_BoneMatrixBuild	( CBoneInstance &bi, const Fmatrix *parent, const SKeyTable	&keys )
{
	// Blend them together
	CKey					channel_keys[MAX_CHANNELS];
	animation::channel_def	BC			[MAX_CHANNELS];
	u16						ch_count = 0;

	for(u16 j= 0;MAX_CHANNELS>j;++j)
	{
		if(j!=0&&keys.chanel_blend_conts[j]==0)
			continue;
		//data for channel mix cycle based on ch_count
		channels.get_def ( j, BC[ch_count]	);
		process_single_channel( channel_keys[ch_count], BC[ch_count], keys.keys[j], keys.blends[j], keys.chanel_blend_conts[j] );
		++ch_count;
	}
	CKey	Result;
	//Mix channels
	MixChannels( Result, channel_keys,  BC, ch_count );

	Fmatrix					RES;
	RES.mk_xform			(Result.Q,Result.T);
	bi.mTransform.mul_43	(*parent,RES);
#ifdef DEBUG
#ifndef _EDITOR
		if(!check_scale(RES))
		{
			VERIFY(check_scale(bi.mTransform));
		}
		VERIFY( _valid( bi.mTransform ) );
		Fbox dbg_box;
		float box_size = 100000.f;
		dbg_box.set( -box_size, -box_size, -box_size, box_size, box_size, box_size );
		//VERIFY(dbg_box.contains(bi.mTransform.c));
		VERIFY2( dbg_box.contains(bi.mTransform.c), ( make_string( "model: %s has strange bone position, matrix : ", getDebugName().c_str() ) + get_string( bi.mTransform ) ).c_str() );

		//if(!is_similar(PrevTransform,RES,0.3f))
		//{
		//	Msg("bone %s",*bd->name)	;
		//}
		//BONE_INST.mPrevTransform.set(RES);
#endif
#endif
}

void	CKinematicsAnimated::BuildBoneMatrix			( const CBoneData* bd, CBoneInstance &bi, const Fmatrix *parent, u8 channel_mask /*= (1<<0)*/ )
{
			
			//CKey				R						[MAX_CHANNELS][MAX_BLENDED];	//all keys 
			//float				BA						[MAX_CHANNELS][MAX_BLENDED];	//all factors
			//int				b_counts				[MAX_CHANNELS]	= {0,0,0,0}; //channel counts
			SKeyTable	keys;
			LL_BuldBoneMatrixDequatize( bd, channel_mask, keys );

			LL_BoneMatrixBuild( bi, parent, keys );

			/*
			if(bi.mTransform.c.y>10000)
			{
			Log("BLEND_INST",BLEND_INST.Blend.size());
			Log("Bone",LL_BoneName_dbg(SelfID));
			Msg("Result.Q %f,%f,%f,%f",Result.Q.x,Result.Q.y,Result.Q.z,Result.Q.w);
			Log("Result.T",Result.T);
			Log("lp parent",(u32)parent);
			Log("parent",*parent);
			Log("RES",RES);
			Log("mT",bi.mTransform);
			CBlend*			B		=	*BI;
			CMotion&		M		=	*LL_GetMotion(B->motionID,SelfID);
			float			time	=	B->timeCurrent*float(SAMPLE_FPS);
			u32				frame	=	iFloor(time);
			u32				count	=	M.get_count();
			float			delta	=	time-float(frame);
			Log("flTKeyPresent",M.test_flag(flTKeyPresent));
			Log("M._initT",M._initT);
			Log("M._sizeT",M._sizeT);
			// translate
			if (M.test_flag(flTKeyPresent))
			{
			CKeyQT*	K1t	= &M._keysT[(frame+0)%count];
			CKeyQT*	K2t	= &M._keysT[(frame+1)%count];
			Fvector T1,T2,Dt;
			T1.x		= float(K1t->x)*M._sizeT.x+M._initT.x;
			T1.y		= float(K1t->y)*M._sizeT.y+M._initT.y;
			T1.z		= float(K1t->z)*M._sizeT.z+M._initT.z;
			T2.x		= float(K2t->x)*M._sizeT.x+M._initT.x;
			T2.y		= float(K2t->y)*M._sizeT.y+M._initT.y;
			T2.z		= float(K2t->z)*M._sizeT.z+M._initT.z;
			Dt.lerp	(T1,T2,delta);
			Msg("K1t %d,%d,%d",K1t->x,K1t->y,K1t->z);
			Msg("K2t %d,%d,%d",K2t->x,K2t->y,K2t->z);
			Log("count",count);
			Log("frame",frame);
			Log("T1",T1);
			Log("T2",T2);
			Log("delta",delta);
			Log("Dt",Dt);
			}else
			{
			D->T.set	(M._initT);
			}
			VERIFY(0);
			}
			*/

}

void CKinematicsAnimated::OnCalculateBones		()
{
	UpdateTracks	()	;
}
IBlendDestroyCallback* CKinematicsAnimated::GetBlendDestroyCallback	( )
{
	return m_blend_destroy_callback;
}

void	CKinematicsAnimated::SetUpdateTracksCalback	( IUpdateTracksCallback	*callback )
{
	m_update_tracks_callback = callback;
}

void	CKinematicsAnimated::SetBlendDestroyCallback		( IBlendDestroyCallback	*cb )
{
	m_blend_destroy_callback = cb;
}

void	CKinematicsAnimated::LL_IterateBlends( IterateBlendsCallback &callback )
{
	CBlend *I=blend_pool.begin(), *E=blend_pool.end();
	for (; I!=E; I++)
		if (I->blend_state() != CBlend::eFREE_SLOT) callback(*I);
}
