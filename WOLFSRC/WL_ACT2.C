// WL_ACT2.C

#include "WL_DEF.H"
#pragma hdrstop

/*
=============================================================================

						 LOCAL CONSTANTS

=============================================================================
*/

#define PROJECTILESIZE	0xc000l

#define BJRUNSPEED	2048
#define BJJUMPSPEED	680


/*
=============================================================================

						 GLOBAL VARIABLES

=============================================================================
*/



/*
=============================================================================

						 LOCAL VARIABLES

=============================================================================
*/


dirtype dirtable[9] = {northwest,north,northeast,west,nodir,east,
	southwest,south,southeast};

int16_t	starthitpoints[4][NUMENEMIES] =
	 //
	 // BABY MODE
	 //
	 {
	 {25,	// guards
	  50,	// officer
	  100,	// SS
	  1,	// dogs
	  850,	// Hans
	  850,	// Schabbs
	  200,	// fake hitler
	  800,	// mecha hitler
	  45,	// mutants
	  25,	// ghosts
	  25,	// ghosts
	  25,	// ghosts
	  25,	// ghosts

	  850,	// Gretel
	  850,	// Gift
	  850,	// Fat
	  5,	// en_spectre,
	  1450,	// en_angel,
	  850,	// en_trans,
	  1050,	// en_uber,
	  950,	// en_will,
	  1250	// en_death
	  },
	 //
	 // DON'T HURT ME MODE
	 //
	 {25,	// guards
	  50,	// officer
	  100,	// SS
	  1,	// dogs
	  950,	// Hans
	  950,	// Schabbs
	  300,	// fake hitler
	  950,	// mecha hitler
	  55,	// mutants
	  25,	// ghosts
	  25,	// ghosts
	  25,	// ghosts
	  25,	// ghosts

	  950,	// Gretel
	  950,	// Gift
	  950,	// Fat
	  10,	// en_spectre,
	  1550,	// en_angel,
	  950,	// en_trans,
	  1150,	// en_uber,
	  1050,	// en_will,
	  1350	// en_death
	  },
	 //
	 // BRING 'EM ON MODE
	 //
	 {25,	// guards
	  50,	// officer
	  100,	// SS
	  1,	// dogs

	  1050,	// Hans
	  1550,	// Schabbs
	  400,	// fake hitler
	  1050,	// mecha hitler

	  55,	// mutants
	  25,	// ghosts
	  25,	// ghosts
	  25,	// ghosts
	  25,	// ghosts

	  1050,	// Gretel
	  1050,	// Gift
	  1050,	// Fat
	  15,	// en_spectre,
	  1650,	// en_angel,
	  1050,	// en_trans,
	  1250,	// en_uber,
	  1150,	// en_will,
	  1450	// en_death
	  },
	 //
	 // DEATH INCARNATE MODE
	 //
	 {25,	// guards
	  50,	// officer
	  100,	// SS
	  1,	// dogs

	  1200,	// Hans
	  2400,	// Schabbs
	  500,	// fake hitler
	  1200,	// mecha hitler

	  65,	// mutants
	  25,	// ghosts
	  25,	// ghosts
	  25,	// ghosts
	  25,	// ghosts

	  1200,	// Gretel
	  1200,	// Gift
	  1200,	// Fat
	  25,	// en_spectre,
	  2000,	// en_angel,
	  1200,	// en_trans,
	  1400,	// en_uber,
	  1300,	// en_will,
	  1600	// en_death
	  }}
	  ;

void	A_StartDeathCam (objtype *ob);


void	T_Path (objtype *ob);
void	T_Shoot (objtype *ob);
void	T_Bite (objtype *ob);
void	T_DogChase (objtype *ob);
void	T_Chase (objtype *ob);
void	T_Projectile (objtype *ob);
void	T_Stand (objtype *ob);

void	A_DeathScream (objtype *ob);

void	A_Smoke(objtype* ob);

void Act2MakeStates()
{
    MAKESTATE(s_rocket,true,SPR_ROCKET_1,3,T_Projectile,A_Smoke,s_rocket);
    MAKESTATE(s_smoke1,false,SPR_SMOKE_1,3,NULL,NULL,s_smoke2);
    MAKESTATE(s_smoke2,false,SPR_SMOKE_2,3,NULL,NULL,s_smoke3);
    MAKESTATE(s_smoke3,false,SPR_SMOKE_3,3,NULL,NULL,s_smoke4);
    MAKESTATE(s_smoke4,false,SPR_SMOKE_4,3,NULL,NULL,NULLSTATE);

    MAKESTATE(s_boom1,false,SPR_BOOM_1,6,NULL,NULL,s_boom2);
    MAKESTATE(s_boom2,false,SPR_BOOM_2,6,NULL,NULL,s_boom3);
    MAKESTATE(s_boom3,false,SPR_BOOM_3,6,NULL,NULL,NULLSTATE);

#ifdef SPEAR

    MAKESTATE(s_hrocket,true,SPR_HROCKET_1,3,T_Projectile,A_Smoke,s_hrocket);
    MAKESTATE(s_hsmoke1,false,SPR_HSMOKE_1,3,NULL,NULL,s_hsmoke2);
    MAKESTATE(s_hsmoke2,false,SPR_HSMOKE_2,3,NULL,NULL,s_hsmoke3);
    MAKESTATE(s_hsmoke3,false,SPR_HSMOKE_3,3,NULL,NULL,s_hsmoke4);
    MAKESTATE(s_hsmoke4,false,SPR_HSMOKE_4,3,NULL,NULL,NULLSTATE);

    MAKESTATE(s_hboom1,false,SPR_HBOOM_1,6,NULL,NULL,s_hboom2);
    MAKESTATE(s_hboom2,false,SPR_HBOOM_2,6,NULL,NULL,s_hboom3);
    MAKESTATE(s_hboom3,false,SPR_HBOOM_3,6,NULL,NULL,NULLSTATE);

#endif
}

void	T_Schabb (objtype *ob);
void	T_SchabbThrow (objtype *ob);
void	T_Fake (objtype *ob);
void	T_FakeFire (objtype *ob);
void	T_Ghosts (objtype *ob);

void	A_Slurpie (objtype *ob);
void	A_HitlerMorph (objtype *ob);
void	A_MechaSound (objtype *ob);

/*
=================
=
= A_Smoke
=
=================
*/

void A_Smoke (objtype *ob)
{
	GetNewActor ();
#ifdef SPEAR
	if (ob->obclass == hrocketobj)
		new->state = s_hsmoke1;
	else
#endif
		new->state = s_smoke1;
	new->ticcount = 6;

	new->tilex = ob->tilex;
	new->tiley = ob->tiley;
	new->x = ob->x;
	new->y = ob->y;
	new->obclass = inertobj;
	new->active = ac_yes;

	new->flags = FL_NEVERMARK;
}


/*
===================
=
= ProjectileTryMove
=
= returns true if move ok
===================
*/

#define PROJSIZE	0x2000

boolean ProjectileTryMove (objtype *ob)
{
	int16_t		xl,yl,xh,yh,x,y;
	uint16_t	check;

	xl = (ob->x-PROJSIZE) >>TILESHIFT;
	yl = (ob->y-PROJSIZE) >>TILESHIFT;

	xh = (ob->x+PROJSIZE) >>TILESHIFT;
	yh = (ob->y+PROJSIZE) >>TILESHIFT;

//
// check for solid walls
//
	for (y=yl;y<=yh;y++)
		for (x=xl;x<=xh;x++)
		{
			check = actorat[x][y];
			if (check && check<256)
				return false;
		}

	return true;
}



/*
=================
=
= T_Projectile
=
=================
*/

void T_Projectile (objtype *ob)
{
	int32_t	deltax,deltay;
	int16_t	damage;
	int32_t	speed;

	speed = (int32_t)ob->speed*tics;

	deltax = FixedByFrac(speed,costable[ob->angle]);
	deltay = -FixedByFrac(speed,sintable[ob->angle]);

	if (deltax>0x10000l)
		deltax = 0x10000l;
	if (deltay>0x10000l)
		deltay = 0x10000l;

	ob->x += deltax;
	ob->y += deltay;

	deltax = LABS(ob->x - player->x);
	deltay = LABS(ob->y - player->y);

	if (!ProjectileTryMove (ob))
	{
		if (ob->obclass == rocketobj)
		{
			PlaySoundLocActor(MISSILEHITSND,ob);
			ob->state = s_boom1;
		}
#ifdef SPEAR
		else if (ob->obclass == hrocketobj)
		{
			PlaySoundLocActor(MISSILEHITSND,ob);
			ob->state = s_hboom1;
		}
#endif
		else
			ob->state = NULLSTATE;		// mark for removal

		return;
	}

	if (deltax < PROJECTILESIZE && deltay < PROJECTILESIZE)
	{	// hit the player
		switch (ob->obclass)
		{
		case needleobj:
			damage = (US_RndT() >>3) + 20;
			break;
		case rocketobj:
		case hrocketobj:
		case sparkobj:
			damage = (US_RndT() >>3) + 30;
			break;
		case fireobj:
			damage = (US_RndT() >>3);
			break;
		}

		TakeDamage (damage,ob);
		ob->state = NULLSTATE;		// mark for removal
		return;
	}

	ob->tilex = ob->x >> TILESHIFT;
	ob->tiley = ob->y >> TILESHIFT;

}




/*
=============================================================================

							GUARD

=============================================================================
*/

//
// guards
//

void Act2GuardsMakeStates()
{
    MAKESTATE(s_grdstand,true,SPR_GRD_S_1,0,T_Stand,NULL,s_grdstand);

    MAKESTATE(s_grdpath1,true,SPR_GRD_W1_1,20,T_Path,NULL,s_grdpath1s);
    MAKESTATE(s_grdpath1s,true,SPR_GRD_W1_1,5,NULL,NULL,s_grdpath2);
    MAKESTATE(s_grdpath2,true,SPR_GRD_W2_1,15,T_Path,NULL,s_grdpath3);
    MAKESTATE(s_grdpath3,true,SPR_GRD_W3_1,20,T_Path,NULL,s_grdpath3s);
    MAKESTATE(s_grdpath3s,true,SPR_GRD_W3_1,5,NULL,NULL,s_grdpath4);
    MAKESTATE(s_grdpath4,true,SPR_GRD_W4_1,15,T_Path,NULL,s_grdpath1);

    MAKESTATE(s_grdpain,2,SPR_GRD_PAIN_1,10,NULL,NULL,s_grdchase1);
    MAKESTATE(s_grdpain1,2,SPR_GRD_PAIN_2,10,NULL,NULL,s_grdchase1);

    MAKESTATE(s_grdshoot1,false,SPR_GRD_SHOOT1,20,NULL,NULL,s_grdshoot2);
    MAKESTATE(s_grdshoot2,false,SPR_GRD_SHOOT2,20,NULL,T_Shoot,s_grdshoot3);
    MAKESTATE(s_grdshoot3,false,SPR_GRD_SHOOT3,20,NULL,NULL,s_grdchase1);

    MAKESTATE(s_grdchase1,true,SPR_GRD_W1_1,10,T_Chase,NULL,s_grdchase1s);
    MAKESTATE(s_grdchase1s,true,SPR_GRD_W1_1,3,NULL,NULL,s_grdchase2);
    MAKESTATE(s_grdchase2,true,SPR_GRD_W2_1,8,T_Chase,NULL,s_grdchase3);
    MAKESTATE(s_grdchase3,true,SPR_GRD_W3_1,10,T_Chase,NULL,s_grdchase3s);
    MAKESTATE(s_grdchase3s,true,SPR_GRD_W3_1,3,NULL,NULL,s_grdchase4);
    MAKESTATE(s_grdchase4,true,SPR_GRD_W4_1,8,T_Chase,NULL,s_grdchase1);

    MAKESTATE(s_grddie1,false,SPR_GRD_DIE_1,15,NULL,A_DeathScream,s_grddie2);
    MAKESTATE(s_grddie2,false,SPR_GRD_DIE_2,15,NULL,NULL,s_grddie3);
    MAKESTATE(s_grddie3,false,SPR_GRD_DIE_3,15,NULL,NULL,s_grddie4);
    MAKESTATE(s_grddie4,false,SPR_GRD_DEAD,0,NULL,NULL,s_grddie4);
}

#ifndef SPEAR
//
// ghosts
//
void Act2GhostsMakeStates()
{
    MAKESTATE(s_blinkychase1,false,SPR_BLINKY_W1,10,T_Ghosts,NULL,s_blinkychase2);
    MAKESTATE(s_blinkychase2,false,SPR_BLINKY_W2,10,T_Ghosts,NULL,s_blinkychase1);

    MAKESTATE(s_inkychase1,false,SPR_INKY_W1,10,T_Ghosts,NULL,s_inkychase2);
    MAKESTATE(s_inkychase2,false,SPR_INKY_W2,10,T_Ghosts,NULL,s_inkychase1);

    MAKESTATE(s_pinkychase1,false,SPR_PINKY_W1,10,T_Ghosts,NULL,s_pinkychase2);
    MAKESTATE(s_pinkychase2,false,SPR_PINKY_W2,10,T_Ghosts,NULL,s_pinkychase1);

    MAKESTATE(s_clydechase1,false,SPR_CLYDE_W1,10,T_Ghosts,NULL,s_clydechase2);
    MAKESTATE(s_clydechase2,false,SPR_CLYDE_W2,10,T_Ghosts,NULL,s_clydechase1);
}
#endif

//
// dogs
//

void Act2DogsMakeStates()
{
    MAKESTATE(s_dogpath1,true,SPR_DOG_W1_1,20,T_Path,NULL,s_dogpath1s);
    MAKESTATE(s_dogpath1s,true,SPR_DOG_W1_1,5,NULL,NULL,s_dogpath2);
    MAKESTATE(s_dogpath2,true,SPR_DOG_W2_1,15,T_Path,NULL,s_dogpath3);
    MAKESTATE(s_dogpath3,true,SPR_DOG_W3_1,20,T_Path,NULL,s_dogpath3s);
    MAKESTATE(s_dogpath3s,true,SPR_DOG_W3_1,5,NULL,NULL,s_dogpath4);
    MAKESTATE(s_dogpath4,true,SPR_DOG_W4_1,15,T_Path,NULL,s_dogpath1);

    MAKESTATE(s_dogjump1,false,SPR_DOG_JUMP1,10,NULL,NULL,s_dogjump2);
    MAKESTATE(s_dogjump2,false,SPR_DOG_JUMP2,10,NULL,T_Bite,s_dogjump3);
    MAKESTATE(s_dogjump3,false,SPR_DOG_JUMP3,10,NULL,NULL,s_dogjump4);
    MAKESTATE(s_dogjump4,false,SPR_DOG_JUMP1,10,NULL,NULL,s_dogjump5);
    MAKESTATE(s_dogjump5,false,SPR_DOG_W1_1,10,NULL,NULL,s_dogchase1);

    MAKESTATE(s_dogchase1,true,SPR_DOG_W1_1,10,T_DogChase,NULL,s_dogchase1s);
    MAKESTATE(s_dogchase1s,true,SPR_DOG_W1_1,3,NULL,NULL,s_dogchase2);
    MAKESTATE(s_dogchase2,true,SPR_DOG_W2_1,8,T_DogChase,NULL,s_dogchase3);
    MAKESTATE(s_dogchase3,true,SPR_DOG_W3_1,10,T_DogChase,NULL,s_dogchase3s);
    MAKESTATE(s_dogchase3s,true,SPR_DOG_W3_1,3,NULL,NULL,s_dogchase4);
    MAKESTATE(s_dogchase4,true,SPR_DOG_W4_1,8,T_DogChase,NULL,s_dogchase1);

    MAKESTATE(s_dogdie1,false,SPR_DOG_DIE_1,15,NULL,A_DeathScream,s_dogdie2);
    MAKESTATE(s_dogdie2,false,SPR_DOG_DIE_2,15,NULL,NULL,s_dogdie3);
    MAKESTATE(s_dogdie3,false,SPR_DOG_DIE_3,15,NULL,NULL,s_dogdead);
    MAKESTATE(s_dogdead,false,SPR_DOG_DEAD,15,NULL,NULL,s_dogdead);
}

//
// officers
//

void Act2OfficersMakeStates()
{
    MAKESTATE(s_ofcstand,true,SPR_OFC_S_1,0,T_Stand,NULL,s_ofcstand);

    MAKESTATE(s_ofcpath1,true,SPR_OFC_W1_1,20,T_Path,NULL,s_ofcpath1s);
    MAKESTATE(s_ofcpath1s,true,SPR_OFC_W1_1,5,NULL,NULL,s_ofcpath2);
    MAKESTATE(s_ofcpath2,true,SPR_OFC_W2_1,15,T_Path,NULL,s_ofcpath3);
    MAKESTATE(s_ofcpath3,true,SPR_OFC_W3_1,20,T_Path,NULL,s_ofcpath3s);
    MAKESTATE(s_ofcpath3s,true,SPR_OFC_W3_1,5,NULL,NULL,s_ofcpath4);
    MAKESTATE(s_ofcpath4,true,SPR_OFC_W4_1,15,T_Path,NULL,s_ofcpath1);

    MAKESTATE(s_ofcpain,2,SPR_OFC_PAIN_1,10,NULL,NULL,s_ofcchase1);
    MAKESTATE(s_ofcpain1,2,SPR_OFC_PAIN_2,10,NULL,NULL,s_ofcchase1);

    MAKESTATE(s_ofcshoot1,false,SPR_OFC_SHOOT1,6,NULL,NULL,s_ofcshoot2);
    MAKESTATE(s_ofcshoot2,false,SPR_OFC_SHOOT2,20,NULL,T_Shoot,s_ofcshoot3);
    MAKESTATE(s_ofcshoot3,false,SPR_OFC_SHOOT3,10,NULL,NULL,s_ofcchase1);

    MAKESTATE(s_ofcchase1,true,SPR_OFC_W1_1,10,T_Chase,NULL,s_ofcchase1s);
    MAKESTATE(s_ofcchase1s,true,SPR_OFC_W1_1,3,NULL,NULL,s_ofcchase2);
    MAKESTATE(s_ofcchase2,true,SPR_OFC_W2_1,8,T_Chase,NULL,s_ofcchase3);
    MAKESTATE(s_ofcchase3,true,SPR_OFC_W3_1,10,T_Chase,NULL,s_ofcchase3s);
    MAKESTATE(s_ofcchase3s,true,SPR_OFC_W3_1,3,NULL,NULL,s_ofcchase4);
    MAKESTATE(s_ofcchase4,true,SPR_OFC_W4_1,8,T_Chase,NULL,s_ofcchase1);

    MAKESTATE(s_ofcdie1,false,SPR_OFC_DIE_1,11,NULL,A_DeathScream,s_ofcdie2);
    MAKESTATE(s_ofcdie2,false,SPR_OFC_DIE_2,11,NULL,NULL,s_ofcdie3);
    MAKESTATE(s_ofcdie3,false,SPR_OFC_DIE_3,11,NULL,NULL,s_ofcdie4);
    MAKESTATE(s_ofcdie4,false,SPR_OFC_DIE_4,11,NULL,NULL,s_ofcdie5);
    MAKESTATE(s_ofcdie5,false,SPR_OFC_DEAD,0,NULL,NULL,s_ofcdie5);
}

//
// mutant
//

void Act2MutantMakeStates()
{
    MAKESTATE(s_mutstand,true,SPR_MUT_S_1,0,T_Stand,NULL,s_mutstand);

    MAKESTATE(s_mutpath1,true,SPR_MUT_W1_1,20,T_Path,NULL,s_mutpath1s);
    MAKESTATE(s_mutpath1s,true,SPR_MUT_W1_1,5,NULL,NULL,s_mutpath2);
    MAKESTATE(s_mutpath2,true,SPR_MUT_W2_1,15,T_Path,NULL,s_mutpath3);
    MAKESTATE(s_mutpath3,true,SPR_MUT_W3_1,20,T_Path,NULL,s_mutpath3s);
    MAKESTATE(s_mutpath3s,true,SPR_MUT_W3_1,5,NULL,NULL,s_mutpath4);
    MAKESTATE(s_mutpath4,true,SPR_MUT_W4_1,15,T_Path,NULL,s_mutpath1);

    MAKESTATE(s_mutpain,2,SPR_MUT_PAIN_1,10,NULL,NULL,s_mutchase1);
    MAKESTATE(s_mutpain1,2,SPR_MUT_PAIN_2,10,NULL,NULL,s_mutchase1);

    MAKESTATE(s_mutshoot1,false,SPR_MUT_SHOOT1,6,NULL,T_Shoot,s_mutshoot2);
    MAKESTATE(s_mutshoot2,false,SPR_MUT_SHOOT2,20,NULL,NULL,s_mutshoot3);
    MAKESTATE(s_mutshoot3,false,SPR_MUT_SHOOT3,10,NULL,T_Shoot,s_mutshoot4);
    MAKESTATE(s_mutshoot4,false,SPR_MUT_SHOOT4,20,NULL,NULL,s_mutchase1);

    MAKESTATE(s_mutchase1,true,SPR_MUT_W1_1,10,T_Chase,NULL,s_mutchase1s);
    MAKESTATE(s_mutchase1s,true,SPR_MUT_W1_1,3,NULL,NULL,s_mutchase2);
    MAKESTATE(s_mutchase2,true,SPR_MUT_W2_1,8,T_Chase,NULL,s_mutchase3);
    MAKESTATE(s_mutchase3,true,SPR_MUT_W3_1,10,T_Chase,NULL,s_mutchase3s);
    MAKESTATE(s_mutchase3s,true,SPR_MUT_W3_1,3,NULL,NULL,s_mutchase4);
    MAKESTATE(s_mutchase4,true,SPR_MUT_W4_1,8,T_Chase,NULL,s_mutchase1);

    MAKESTATE(s_mutdie1,false,SPR_MUT_DIE_1,7,NULL,A_DeathScream,s_mutdie2);
    MAKESTATE(s_mutdie2,false,SPR_MUT_DIE_2,7,NULL,NULL,s_mutdie3);
    MAKESTATE(s_mutdie3,false,SPR_MUT_DIE_3,7,NULL,NULL,s_mutdie4);
    MAKESTATE(s_mutdie4,false,SPR_MUT_DIE_4,7,NULL,NULL,s_mutdie5);
    MAKESTATE(s_mutdie5,false,SPR_MUT_DEAD,0,NULL,NULL,s_mutdie5);
}

//
// SS
//

void Act2SSMakeStates()
{
    MAKESTATE(s_ssstand,true,SPR_SS_S_1,0,T_Stand,NULL,s_ssstand);

    MAKESTATE(s_sspath1,true,SPR_SS_W1_1,20,T_Path,NULL,s_sspath1s);
    MAKESTATE(s_sspath1s,true,SPR_SS_W1_1,5,NULL,NULL,s_sspath2);
    MAKESTATE(s_sspath2,true,SPR_SS_W2_1,15,T_Path,NULL,s_sspath3);
    MAKESTATE(s_sspath3,true,SPR_SS_W3_1,20,T_Path,NULL,s_sspath3s);
    MAKESTATE(s_sspath3s,true,SPR_SS_W3_1,5,NULL,NULL,s_sspath4);
    MAKESTATE(s_sspath4,true,SPR_SS_W4_1,15,T_Path,NULL,s_sspath1);

    MAKESTATE(s_sspain,2,SPR_SS_PAIN_1,10,NULL,NULL,s_sschase1);
    MAKESTATE(s_sspain1,2,SPR_SS_PAIN_2,10,NULL,NULL,s_sschase1);

    MAKESTATE(s_ssshoot1,false,SPR_SS_SHOOT1,20,NULL,NULL,s_ssshoot2);
    MAKESTATE(s_ssshoot2,false,SPR_SS_SHOOT2,20,NULL,T_Shoot,s_ssshoot3);
    MAKESTATE(s_ssshoot3,false,SPR_SS_SHOOT3,10,NULL,NULL,s_ssshoot4);
    MAKESTATE(s_ssshoot4,false,SPR_SS_SHOOT2,10,NULL,T_Shoot,s_ssshoot5);
    MAKESTATE(s_ssshoot5,false,SPR_SS_SHOOT3,10,NULL,NULL,s_ssshoot6);
    MAKESTATE(s_ssshoot6,false,SPR_SS_SHOOT2,10,NULL,T_Shoot,s_ssshoot7);
    MAKESTATE(s_ssshoot7,false,SPR_SS_SHOOT3,10,NULL,NULL,s_ssshoot8);
    MAKESTATE(s_ssshoot8,false,SPR_SS_SHOOT2,10,NULL,T_Shoot,s_ssshoot9);
    MAKESTATE(s_ssshoot9,false,SPR_SS_SHOOT3,10,NULL,NULL,s_sschase1);

    MAKESTATE(s_sschase1,true,SPR_SS_W1_1,10,T_Chase,NULL,s_sschase1s);
    MAKESTATE(s_sschase1s,true,SPR_SS_W1_1,3,NULL,NULL,s_sschase2);
    MAKESTATE(s_sschase2,true,SPR_SS_W2_1,8,T_Chase,NULL,s_sschase3);
    MAKESTATE(s_sschase3,true,SPR_SS_W3_1,10,T_Chase,NULL,s_sschase3s);
    MAKESTATE(s_sschase3s,true,SPR_SS_W3_1,3,NULL,NULL,s_sschase4);
    MAKESTATE(s_sschase4,true,SPR_SS_W4_1,8,T_Chase,NULL,s_sschase1);

    MAKESTATE(s_ssdie1,false,SPR_SS_DIE_1,15,NULL,A_DeathScream,s_ssdie2);
    MAKESTATE(s_ssdie2,false,SPR_SS_DIE_2,15,NULL,NULL,s_ssdie3);
    MAKESTATE(s_ssdie3,false,SPR_SS_DIE_3,15,NULL,NULL,s_ssdie4);
    MAKESTATE(s_ssdie4,false,SPR_SS_DEAD,0,NULL,NULL,s_ssdie4);
}

#ifndef SPEAR
//
// hans
//
void Act2HansMakeStates()
{
    MAKESTATE(s_bossstand,false,SPR_BOSS_W1,0,T_Stand,NULL,s_bossstand);

    MAKESTATE(s_bosschase1,false,SPR_BOSS_W1,10,T_Chase,NULL,s_bosschase1s);
    MAKESTATE(s_bosschase1s,false,SPR_BOSS_W1,3,NULL,NULL,s_bosschase2);
    MAKESTATE(s_bosschase2,false,SPR_BOSS_W2,8,T_Chase,NULL,s_bosschase3);
    MAKESTATE(s_bosschase3,false,SPR_BOSS_W3,10,T_Chase,NULL,s_bosschase3s);
    MAKESTATE(s_bosschase3s,false,SPR_BOSS_W3,3,NULL,NULL,s_bosschase4);
    MAKESTATE(s_bosschase4,false,SPR_BOSS_W4,8,T_Chase,NULL,s_bosschase1);

    MAKESTATE(s_bossdie1,false,SPR_BOSS_DIE1,15,NULL,A_DeathScream,s_bossdie2);
    MAKESTATE(s_bossdie2,false,SPR_BOSS_DIE2,15,NULL,NULL,s_bossdie3);
    MAKESTATE(s_bossdie3,false,SPR_BOSS_DIE3,15,NULL,NULL,s_bossdie4);
    MAKESTATE(s_bossdie4,false,SPR_BOSS_DEAD,0,NULL,NULL,s_bossdie4);

    MAKESTATE(s_bossshoot1,false,SPR_BOSS_SHOOT1,30,NULL,NULL,s_bossshoot2);
    MAKESTATE(s_bossshoot2,false,SPR_BOSS_SHOOT2,10,NULL,T_Shoot,s_bossshoot3);
    MAKESTATE(s_bossshoot3,false,SPR_BOSS_SHOOT3,10,NULL,T_Shoot,s_bossshoot4);
    MAKESTATE(s_bossshoot4,false,SPR_BOSS_SHOOT2,10,NULL,T_Shoot,s_bossshoot5);
    MAKESTATE(s_bossshoot5,false,SPR_BOSS_SHOOT3,10,NULL,T_Shoot,s_bossshoot6);
    MAKESTATE(s_bossshoot6,false,SPR_BOSS_SHOOT2,10,NULL,T_Shoot,s_bossshoot7);
    MAKESTATE(s_bossshoot7,false,SPR_BOSS_SHOOT3,10,NULL,T_Shoot,s_bossshoot8);
    MAKESTATE(s_bossshoot8,false,SPR_BOSS_SHOOT1,10,NULL,NULL,s_bosschase1);
}

//
// gretel
//
void Act2GretelMakeStates()
{
    MAKESTATE(s_gretelstand,false,SPR_GRETEL_W1,0,T_Stand,NULL,s_gretelstand);

    MAKESTATE(s_gretelchase1,false,SPR_GRETEL_W1,10,T_Chase,NULL,s_gretelchase1s);
    MAKESTATE(s_gretelchase1s,false,SPR_GRETEL_W1,3,NULL,NULL,s_gretelchase2);
    MAKESTATE(s_gretelchase2,false,SPR_GRETEL_W2,8,T_Chase,NULL,s_gretelchase3);
    MAKESTATE(s_gretelchase3,false,SPR_GRETEL_W3,10,T_Chase,NULL,s_gretelchase3s);
    MAKESTATE(s_gretelchase3s,false,SPR_GRETEL_W3,3,NULL,NULL,s_gretelchase4);
    MAKESTATE(s_gretelchase4,false,SPR_GRETEL_W4,8,T_Chase,NULL,s_gretelchase1);

    MAKESTATE(s_greteldie1,false,SPR_GRETEL_DIE1,15,NULL,A_DeathScream,s_greteldie2);
    MAKESTATE(s_greteldie2,false,SPR_GRETEL_DIE2,15,NULL,NULL,s_greteldie3);
    MAKESTATE(s_greteldie3,false,SPR_GRETEL_DIE3,15,NULL,NULL,s_greteldie4);
    MAKESTATE(s_greteldie4,false,SPR_GRETEL_DEAD,0,NULL,NULL,s_greteldie4);

    MAKESTATE(s_gretelshoot1,false,SPR_GRETEL_SHOOT1,30,NULL,NULL,s_gretelshoot2);
    MAKESTATE(s_gretelshoot2,false,SPR_GRETEL_SHOOT2,10,NULL,T_Shoot,s_gretelshoot3);
    MAKESTATE(s_gretelshoot3,false,SPR_GRETEL_SHOOT3,10,NULL,T_Shoot,s_gretelshoot4);
    MAKESTATE(s_gretelshoot4,false,SPR_GRETEL_SHOOT2,10,NULL,T_Shoot,s_gretelshoot5);
    MAKESTATE(s_gretelshoot5,false,SPR_GRETEL_SHOOT3,10,NULL,T_Shoot,s_gretelshoot6);
    MAKESTATE(s_gretelshoot6,false,SPR_GRETEL_SHOOT2,10,NULL,T_Shoot,s_gretelshoot7);
    MAKESTATE(s_gretelshoot7,false,SPR_GRETEL_SHOOT3,10,NULL,T_Shoot,s_gretelshoot8);
    MAKESTATE(s_gretelshoot8,false,SPR_GRETEL_SHOOT1,10,NULL,NULL,s_gretelchase1);
}
#endif


/*
===============
=
= SpawnStand
=
===============
*/

void SpawnStand (enemy_t which, int16_t tilex, int16_t tiley, int16_t dir)
{
	uint16_t	*map,tile;

	switch (which)
	{
	case en_guard:
		SpawnNewObj (tilex,tiley,s_grdstand);
		new->speed = SPDPATROL;
		if (!loadedgame)
		  gamestate.killtotal++;
		break;

	case en_officer:
		SpawnNewObj (tilex,tiley,s_ofcstand);
		new->speed = SPDPATROL;
		if (!loadedgame)
		  gamestate.killtotal++;
		break;

	case en_mutant:
		SpawnNewObj (tilex,tiley,s_mutstand);
		new->speed = SPDPATROL;
		if (!loadedgame)
		  gamestate.killtotal++;
		break;

	case en_ss:
		SpawnNewObj (tilex,tiley,s_ssstand);
		new->speed = SPDPATROL;
		if (!loadedgame)
		  gamestate.killtotal++;
		break;
	}


	map = mapsegs[0]+farmapylookup[tiley]+tilex;
	if (*map == AMBUSHTILE)
	{
		tilemap[tilex][tiley] = 0;

		if (*(map+1) >= AREATILE)
			tile = *(map+1);
		if (*(map-mapwidth) >= AREATILE)
			tile = *(map-mapwidth);
		if (*(map+mapwidth) >= AREATILE)
			tile = *(map+mapwidth);
		if ( *(map-1) >= AREATILE)
			tile = *(map-1);

		*map = tile;
		new->areanumber = tile-AREATILE;

		new->flags |= FL_AMBUSH;
	}

	new->obclass = guardobj+which;
	new->hitpoints = starthitpoints[gamestate.difficulty][which];
	new->dir = dir*2;
	new->flags |= FL_SHOOTABLE;
}



/*
===============
=
= SpawnDeadGuard
=
===============
*/

void SpawnDeadGuard (int16_t tilex, int16_t tiley)
{
	SpawnNewObj (tilex,tiley,s_grddie4);
	new->obclass = inertobj;
}



#ifndef SPEAR
/*
===============
=
= SpawnBoss
=
===============
*/

void SpawnBoss (int16_t tilex, int16_t tiley)
{
	SpawnNewObj (tilex,tiley,s_bossstand);
	new->speed = SPDPATROL;

	new->obclass = bossobj;
	new->hitpoints = starthitpoints[gamestate.difficulty][en_boss];
	new->dir = south;
	new->flags |= FL_SHOOTABLE|FL_AMBUSH;
	if (!loadedgame)
	  gamestate.killtotal++;
}

/*
===============
=
= SpawnGretel
=
===============
*/

void SpawnGretel (int16_t tilex, int16_t tiley)
{
	SpawnNewObj (tilex,tiley,s_gretelstand);
	new->speed = SPDPATROL;

	new->obclass = gretelobj;
	new->hitpoints = starthitpoints[gamestate.difficulty][en_gretel];
	new->dir = north;
	new->flags |= FL_SHOOTABLE|FL_AMBUSH;
	if (!loadedgame)
	  gamestate.killtotal++;
}
#endif

/*
===============
=
= SpawnPatrol
=
===============
*/

void SpawnPatrol (enemy_t which, int16_t tilex, int16_t tiley, int16_t dir)
{
    int16_t actorindex;

	switch (which)
	{
	case en_guard:
		SpawnNewObj (tilex,tiley,s_grdpath1);
		new->speed = SPDPATROL;
		if (!loadedgame)
		  gamestate.killtotal++;
		break;

	case en_officer:
		SpawnNewObj (tilex,tiley,s_ofcpath1);
		new->speed = SPDPATROL;
		if (!loadedgame)
		  gamestate.killtotal++;
		break;

	case en_ss:
		SpawnNewObj (tilex,tiley,s_sspath1);
		new->speed = SPDPATROL;
		if (!loadedgame)
		  gamestate.killtotal++;
		break;

	case en_mutant:
		SpawnNewObj (tilex,tiley,s_mutpath1);
		new->speed = SPDPATROL;
		if (!loadedgame)
		  gamestate.killtotal++;
		break;

	case en_dog:
		SpawnNewObj (tilex,tiley,s_dogpath1);
		new->speed = SPDDOG;
		if (!loadedgame)
		  gamestate.killtotal++;
		break;
	}

	new->obclass = guardobj+which;
	new->dir = dir*2;
	new->hitpoints = starthitpoints[gamestate.difficulty][which];
	new->distance = tileglobal;
	new->flags |= FL_SHOOTABLE;
	new->active = ac_yes;

	actorat[new->tilex][new->tiley] = 0;		// don't use original spot

	switch (dir)
	{
	case 0:
		new->tilex++;
		break;
	case 1:
		new->tiley--;
		break;
	case 2:
		new->tilex--;
		break;
	case 3:
		new->tiley++;
		break;
	}

    actorindex = new - objlist;
    if (actorindex < 0 || actorindex >= MAXACTORS)
        Quit("SpawnPatrol: Bad actor index");

    actorat[new->tilex][new->tiley] = ACTORID(actorindex);
}



/*
==================
=
= A_DeathScream
=
==================
*/

void A_DeathScream (objtype *ob)
{
#ifndef UPLOAD
#ifndef SPEAR
	if (mapon==9 && !US_RndT())
#else
	if ((mapon==18 || mapon==19) && !US_RndT())
#endif
	{
	 switch(ob->obclass)
	 {
	  case mutantobj:
	  case guardobj:
	  case officerobj:
	  case ssobj:
	  case dogobj:
		PlaySoundLocActor(DEATHSCREAM6SND,ob);
		return;
	 }
	}
#endif

	switch (ob->obclass)
	{
	case mutantobj:
		PlaySoundLocActor(AHHHGSND,ob);
		break;

	case guardobj:
		{
		 int16_t sounds[9]={ DEATHSCREAM1SND,
				 DEATHSCREAM2SND,
				 DEATHSCREAM3SND,
				 DEATHSCREAM4SND,
				 DEATHSCREAM5SND,
				 DEATHSCREAM7SND,
				 DEATHSCREAM8SND,
				 DEATHSCREAM9SND
				 };

		 #ifndef UPLOAD
		 PlaySoundLocActor(sounds[US_RndT()%8],ob);
		 #else
		 PlaySoundLocActor(sounds[US_RndT()%2],ob);
		 #endif
		}
		break;
	case officerobj:
		PlaySoundLocActor(NEINSOVASSND,ob);
		break;
	case ssobj:
		PlaySoundLocActor(LEBENSND,ob);	// JAB
		break;
	case dogobj:
		PlaySoundLocActor(DOGDEATHSND,ob);	// JAB
		break;
#ifndef SPEAR
	case bossobj:
		SD_PlaySound(MUTTISND);				// JAB
		break;
	case schabbobj:
		SD_PlaySound(MEINGOTTSND);
		break;
	case fakeobj:
		SD_PlaySound(HITLERHASND);
		break;
	case mechahitlerobj:
		SD_PlaySound(SCHEISTSND);
		break;
	case realhitlerobj:
		SD_PlaySound(EVASND);
		break;
	case gretelobj:
		SD_PlaySound(MEINSND);
		break;
	case giftobj:
		SD_PlaySound(DONNERSND);
		break;
	case fatobj:
		SD_PlaySound(ROSESND);
		break;
#else
	case spectreobj:
		SD_PlaySound(GHOSTFADESND);
		break;
	case angelobj:
		SD_PlaySound(ANGELDEATHSND);
		break;
	case transobj:
		SD_PlaySound(TRANSDEATHSND);
		break;
	case uberobj:
		SD_PlaySound(UBERDEATHSND);
		break;
	case willobj:
		SD_PlaySound(WILHELMDEATHSND);
		break;
	case deathobj:
		SD_PlaySound(KNIGHTDEATHSND);
		break;
#endif
	}
}


/*
=============================================================================

						 SPEAR ACTORS

=============================================================================
*/

#ifdef SPEAR

void T_Launch (objtype *ob);
void T_Will (objtype *ob);

//
// trans
//
statetype s_transstand	= {false,SPR_TRANS_W1,0,T_Stand,NULL,&s_transstand};

statetype s_transchase1 	= {false,SPR_TRANS_W1,10,T_Chase,NULL,&s_transchase1s};
statetype s_transchase1s	= {false,SPR_TRANS_W1,3,NULL,NULL,&s_transchase2};
statetype s_transchase2 	= {false,SPR_TRANS_W2,8,T_Chase,NULL,&s_transchase3};
statetype s_transchase3 	= {false,SPR_TRANS_W3,10,T_Chase,NULL,&s_transchase3s};
statetype s_transchase3s	= {false,SPR_TRANS_W3,3,NULL,NULL,&s_transchase4};
statetype s_transchase4 	= {false,SPR_TRANS_W4,8,T_Chase,NULL,&s_transchase1};

statetype s_transdie0	= {false,SPR_TRANS_W1,1,NULL,A_DeathScream,&s_transdie01};
statetype s_transdie01	= {false,SPR_TRANS_W1,1,NULL,NULL,&s_transdie1};
statetype s_transdie1	= {false,SPR_TRANS_DIE1,15,NULL,NULL,&s_transdie2};
statetype s_transdie2	= {false,SPR_TRANS_DIE2,15,NULL,NULL,&s_transdie3};
statetype s_transdie3	= {false,SPR_TRANS_DIE3,15,NULL,NULL,&s_transdie4};
statetype s_transdie4	= {false,SPR_TRANS_DEAD,0,NULL,NULL,&s_transdie4};

statetype s_transshoot1 	= {false,SPR_TRANS_SHOOT1,30,NULL,NULL,&s_transshoot2};
statetype s_transshoot2 	= {false,SPR_TRANS_SHOOT2,10,NULL,T_Shoot,&s_transshoot3};
statetype s_transshoot3 	= {false,SPR_TRANS_SHOOT3,10,NULL,T_Shoot,&s_transshoot4};
statetype s_transshoot4 	= {false,SPR_TRANS_SHOOT2,10,NULL,T_Shoot,&s_transshoot5};
statetype s_transshoot5 	= {false,SPR_TRANS_SHOOT3,10,NULL,T_Shoot,&s_transshoot6};
statetype s_transshoot6 	= {false,SPR_TRANS_SHOOT2,10,NULL,T_Shoot,&s_transshoot7};
statetype s_transshoot7 	= {false,SPR_TRANS_SHOOT3,10,NULL,T_Shoot,&s_transshoot8};
statetype s_transshoot8 	= {false,SPR_TRANS_SHOOT1,10,NULL,NULL,&s_transchase1};


/*
===============
=
= SpawnTrans
=
===============
*/

void SpawnTrans (int16_t tilex, int16_t tiley)
{
	uint16_t	far *map,tile;

	if (SoundBlasterPresent && DigiMode != sds_Off)
		s_transdie01.tictime = 105;

	SpawnNewObj (tilex,tiley,&s_transstand);
	new->obclass = transobj;
	new->hitpoints = starthitpoints[gamestate.difficulty][en_trans];
	new->flags |= FL_SHOOTABLE|FL_AMBUSH;
	if (!loadedgame)
	  gamestate.killtotal++;
}


//
// uber
//
void T_UShoot (objtype *ob);

statetype s_uberstand	= {false,SPR_UBER_W1,0,T_Stand,NULL,&s_uberstand};

statetype s_uberchase1 	= {false,SPR_UBER_W1,10,T_Chase,NULL,&s_uberchase1s};
statetype s_uberchase1s	= {false,SPR_UBER_W1,3,NULL,NULL,&s_uberchase2};
statetype s_uberchase2 	= {false,SPR_UBER_W2,8,T_Chase,NULL,&s_uberchase3};
statetype s_uberchase3 	= {false,SPR_UBER_W3,10,T_Chase,NULL,&s_uberchase3s};
statetype s_uberchase3s	= {false,SPR_UBER_W3,3,NULL,NULL,&s_uberchase4};
statetype s_uberchase4 	= {false,SPR_UBER_W4,8,T_Chase,NULL,&s_uberchase1};

statetype s_uberdie0	= {false,SPR_UBER_W1,1,NULL,A_DeathScream,&s_uberdie01};
statetype s_uberdie01	= {false,SPR_UBER_W1,1,NULL,NULL,&s_uberdie1};
statetype s_uberdie1	= {false,SPR_UBER_DIE1,15,NULL,NULL,&s_uberdie2};
statetype s_uberdie2	= {false,SPR_UBER_DIE2,15,NULL,NULL,&s_uberdie3};
statetype s_uberdie3	= {false,SPR_UBER_DIE3,15,NULL,NULL,&s_uberdie4};
statetype s_uberdie4	= {false,SPR_UBER_DIE4,15,NULL,NULL,&s_uberdie5};
statetype s_uberdie5	= {false,SPR_UBER_DEAD,0,NULL,NULL,&s_uberdie5};

statetype s_ubershoot1 	= {false,SPR_UBER_SHOOT1,30,NULL,NULL,&s_ubershoot2};
statetype s_ubershoot2 	= {false,SPR_UBER_SHOOT2,12,NULL,T_UShoot,&s_ubershoot3};
statetype s_ubershoot3 	= {false,SPR_UBER_SHOOT3,12,NULL,T_UShoot,&s_ubershoot4};
statetype s_ubershoot4 	= {false,SPR_UBER_SHOOT4,12,NULL,T_UShoot,&s_ubershoot5};
statetype s_ubershoot5 	= {false,SPR_UBER_SHOOT3,12,NULL,T_UShoot,&s_ubershoot6};
statetype s_ubershoot6 	= {false,SPR_UBER_SHOOT2,12,NULL,T_UShoot,&s_ubershoot7};
statetype s_ubershoot7 	= {false,SPR_UBER_SHOOT1,12,NULL,NULL,&s_uberchase1};


/*
===============
=
= SpawnUber
=
===============
*/

void SpawnUber (int16_t tilex, int16_t tiley)
{
	uint16_t	far *map,tile;

	if (SoundBlasterPresent && DigiMode != sds_Off)
		s_uberdie01.tictime = 70;

	SpawnNewObj (tilex,tiley,&s_uberstand);
	new->obclass = uberobj;
	new->hitpoints = starthitpoints[gamestate.difficulty][en_uber];
	new->flags |= FL_SHOOTABLE|FL_AMBUSH;
	if (!loadedgame)
	  gamestate.killtotal++;
}


/*
===============
=
= T_UShoot
=
===============
*/

void T_UShoot (objtype *ob)
{
	int16_t	dx,dy,dist;

	T_Shoot (ob);

	dx = abs(ob->tilex - player->tilex);
	dy = abs(ob->tiley - player->tiley);
	dist = dx>dy ? dx : dy;
	if (dist <= 1)
		TakeDamage (10,ob);
}


//
// will
//
statetype s_willstand	= {false,SPR_WILL_W1,0,T_Stand,NULL,&s_willstand};

statetype s_willchase1 	= {false,SPR_WILL_W1,10,T_Will,NULL,&s_willchase1s};
statetype s_willchase1s	= {false,SPR_WILL_W1,3,NULL,NULL,&s_willchase2};
statetype s_willchase2 	= {false,SPR_WILL_W2,8,T_Will,NULL,&s_willchase3};
statetype s_willchase3 	= {false,SPR_WILL_W3,10,T_Will,NULL,&s_willchase3s};
statetype s_willchase3s	= {false,SPR_WILL_W3,3,NULL,NULL,&s_willchase4};
statetype s_willchase4 	= {false,SPR_WILL_W4,8,T_Will,NULL,&s_willchase1};

statetype s_willdeathcam	= {false,SPR_WILL_W1,1,NULL,NULL,&s_willdie1};

statetype s_willdie1	= {false,SPR_WILL_W1,1,NULL,A_DeathScream,&s_willdie2};
statetype s_willdie2	= {false,SPR_WILL_W1,10,NULL,NULL,&s_willdie3};
statetype s_willdie3	= {false,SPR_WILL_DIE1,10,NULL,NULL,&s_willdie4};
statetype s_willdie4	= {false,SPR_WILL_DIE2,10,NULL,NULL,&s_willdie5};
statetype s_willdie5	= {false,SPR_WILL_DIE3,10,NULL,NULL,&s_willdie6};
statetype s_willdie6	= {false,SPR_WILL_DEAD,20,NULL,NULL,&s_willdie6};

statetype s_willshoot1 	= {false,SPR_WILL_SHOOT1,30,NULL,NULL,&s_willshoot2};
statetype s_willshoot2 	= {false,SPR_WILL_SHOOT2,10,NULL,T_Launch,&s_willshoot3};
statetype s_willshoot3 	= {false,SPR_WILL_SHOOT3,10,NULL,T_Shoot,&s_willshoot4};
statetype s_willshoot4 	= {false,SPR_WILL_SHOOT4,10,NULL,T_Shoot,&s_willshoot5};
statetype s_willshoot5 	= {false,SPR_WILL_SHOOT3,10,NULL,T_Shoot,&s_willshoot6};
statetype s_willshoot6 	= {false,SPR_WILL_SHOOT4,10,NULL,T_Shoot,&s_willchase1};


/*
===============
=
= SpawnWill
=
===============
*/

void SpawnWill (int16_t tilex, int16_t tiley)
{
	uint16_t	far *map,tile;

	if (SoundBlasterPresent && DigiMode != sds_Off)
		s_willdie2.tictime = 70;

	SpawnNewObj (tilex,tiley,&s_willstand);
	new->obclass = willobj;
	new->hitpoints = starthitpoints[gamestate.difficulty][en_will];
	new->flags |= FL_SHOOTABLE|FL_AMBUSH;
	if (!loadedgame)
	  gamestate.killtotal++;
}


/*
================
=
= T_Will
=
================
*/

void T_Will (objtype *ob)
{
	int32_t move;
	int16_t	dx,dy,dist;
	boolean	dodge;

	dodge = false;
	dx = abs(ob->tilex - player->tilex);
	dy = abs(ob->tiley - player->tiley);
	dist = dx>dy ? dx : dy;

	if (CheckLine(ob))						// got a shot at player?
	{
		if ( US_RndT() < (tics<<3) )
		{
		//
		// go into attack frame
		//
			if (ob->obclass == willobj)
				NewState (ob,&s_willshoot1);
			else if (ob->obclass == angelobj)
				NewState (ob,&s_angelshoot1);
			else
				NewState (ob,&s_deathshoot1);
			return;
		}
		dodge = true;
	}

	if (ob->dir == nodir)
	{
		if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);
		if (ob->dir == nodir)
			return;							// object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (ob->distance < 0)
		{
		//
		// waiting for a door to open
		//
			OpenDoor (-ob->distance-1);
			if (doorobjlist[-ob->distance-1].action != dr_open)
				return;
			ob->distance = TILEGLOBAL;	// go ahead, the door is now opoen
		}

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		if (dist <4)
			SelectRunDir (ob);
		else if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);

		if (ob->dir == nodir)
			return;							// object is blocked in
	}

}


//
// death
//
statetype s_deathstand	= {false,SPR_DEATH_W1,0,T_Stand,NULL,&s_deathstand};

statetype s_deathchase1 	= {false,SPR_DEATH_W1,10,T_Will,NULL,&s_deathchase1s};
statetype s_deathchase1s	= {false,SPR_DEATH_W1,3,NULL,NULL,&s_deathchase2};
statetype s_deathchase2 	= {false,SPR_DEATH_W2,8,T_Will,NULL,&s_deathchase3};
statetype s_deathchase3 	= {false,SPR_DEATH_W3,10,T_Will,NULL,&s_deathchase3s};
statetype s_deathchase3s	= {false,SPR_DEATH_W3,3,NULL,NULL,&s_deathchase4};
statetype s_deathchase4 	= {false,SPR_DEATH_W4,8,T_Will,NULL,&s_deathchase1};

statetype s_deathdeathcam	= {false,SPR_DEATH_W1,1,NULL,NULL,&s_deathdie1};

statetype s_deathdie1	= {false,SPR_DEATH_W1,1,NULL,A_DeathScream,&s_deathdie2};
statetype s_deathdie2	= {false,SPR_DEATH_W1,10,NULL,NULL,&s_deathdie3};
statetype s_deathdie3	= {false,SPR_DEATH_DIE1,10,NULL,NULL,&s_deathdie4};
statetype s_deathdie4	= {false,SPR_DEATH_DIE2,10,NULL,NULL,&s_deathdie5};
statetype s_deathdie5	= {false,SPR_DEATH_DIE3,10,NULL,NULL,&s_deathdie6};
statetype s_deathdie6	= {false,SPR_DEATH_DIE4,10,NULL,NULL,&s_deathdie7};
statetype s_deathdie7	= {false,SPR_DEATH_DIE5,10,NULL,NULL,&s_deathdie8};
statetype s_deathdie8	= {false,SPR_DEATH_DIE6,10,NULL,NULL,&s_deathdie9};
statetype s_deathdie9	= {false,SPR_DEATH_DEAD,0,NULL,NULL,&s_deathdie9};

statetype s_deathshoot1 	= {false,SPR_DEATH_SHOOT1,30,NULL,NULL,&s_deathshoot2};
statetype s_deathshoot2 	= {false,SPR_DEATH_SHOOT2,10,NULL,T_Launch,&s_deathshoot3};
statetype s_deathshoot3 	= {false,SPR_DEATH_SHOOT4,10,NULL,T_Shoot,&s_deathshoot4};
statetype s_deathshoot4 	= {false,SPR_DEATH_SHOOT3,10,NULL,T_Launch,&s_deathshoot5};
statetype s_deathshoot5 	= {false,SPR_DEATH_SHOOT4,10,NULL,T_Shoot,&s_deathchase1};


/*
===============
=
= SpawnDeath
=
===============
*/

void SpawnDeath (int16_t tilex, int16_t tiley)
{
	uint16_t	far *map,tile;

	if (SoundBlasterPresent && DigiMode != sds_Off)
		s_deathdie2.tictime = 105;

	SpawnNewObj (tilex,tiley,&s_deathstand);
	new->obclass = deathobj;
	new->hitpoints = starthitpoints[gamestate.difficulty][en_death];
	new->flags |= FL_SHOOTABLE|FL_AMBUSH;
	if (!loadedgame)
	  gamestate.killtotal++;
}

/*
===============
=
= T_Launch
=
===============
*/

void T_Launch (objtype *ob)
{
	int32_t	deltax,deltay;
	float	angle;
	int16_t		iangle;

	deltax = player->x - ob->x;
	deltay = ob->y - player->y;
	angle = atan2 (deltay,deltax);
	if (angle<0)
		angle = M_PI*2+angle;
	iangle = angle/(M_PI*2)*ANGLES;
	if (ob->obclass == deathobj)
	{
		T_Shoot (ob);
		if (ob->state == &s_deathshoot2)
		{
			iangle-=4;
			if (iangle<0)
				iangle+=ANGLES;
		}
		else
		{
			iangle+=4;
			if (iangle>=ANGLES)
				iangle-=ANGLES;
		}
	}

	GetNewActor ();
	new->state = &s_rocket;
	new->ticcount = 1;

	new->tilex = ob->tilex;
	new->tiley = ob->tiley;
	new->x = ob->x;
	new->y = ob->y;
	new->obclass = rocketobj;
	switch(ob->obclass)
	{
	case deathobj:
		new->state = &s_hrocket;
		new->obclass = hrocketobj;
		PlaySoundLocActor (KNIGHTMISSILESND,new);
		break;
	case angelobj:
		new->state = &s_spark1;
		new->obclass = sparkobj;
		PlaySoundLocActor (ANGELFIRESND,new);
		break;
	default:
		PlaySoundLocActor (MISSILEFIRESND,new);
	}

	new->dir = nodir;
	new->angle = iangle;
	new->speed = 0x2000l;
	new->flags = FL_NONMARK;
	new->active = true;
}



//
// angel
//
void A_Relaunch (objtype *ob);
void A_Victory (objtype *ob);
void A_StartAttack (objtype *ob);
void A_Breathing (objtype *ob);

statetype s_angelstand	= {false,SPR_ANGEL_W1,0,T_Stand,NULL,&s_angelstand};

statetype s_angelchase1 	= {false,SPR_ANGEL_W1,10,T_Will,NULL,&s_angelchase1s};
statetype s_angelchase1s	= {false,SPR_ANGEL_W1,3,NULL,NULL,&s_angelchase2};
statetype s_angelchase2 	= {false,SPR_ANGEL_W2,8,T_Will,NULL,&s_angelchase3};
statetype s_angelchase3 	= {false,SPR_ANGEL_W3,10,T_Will,NULL,&s_angelchase3s};
statetype s_angelchase3s	= {false,SPR_ANGEL_W3,3,NULL,NULL,&s_angelchase4};
statetype s_angelchase4 	= {false,SPR_ANGEL_W4,8,T_Will,NULL,&s_angelchase1};

statetype s_angeldie1	= {false,SPR_ANGEL_W1,1,NULL,A_DeathScream,&s_angeldie11};
statetype s_angeldie11	= {false,SPR_ANGEL_W1,1,NULL,NULL,&s_angeldie2};
statetype s_angeldie2	= {false,SPR_ANGEL_DIE1,10,NULL,A_Slurpie,&s_angeldie3};
statetype s_angeldie3	= {false,SPR_ANGEL_DIE2,10,NULL,NULL,&s_angeldie4};
statetype s_angeldie4	= {false,SPR_ANGEL_DIE3,10,NULL,NULL,&s_angeldie5};
statetype s_angeldie5	= {false,SPR_ANGEL_DIE4,10,NULL,NULL,&s_angeldie6};
statetype s_angeldie6	= {false,SPR_ANGEL_DIE5,10,NULL,NULL,&s_angeldie7};
statetype s_angeldie7	= {false,SPR_ANGEL_DIE6,10,NULL,NULL,&s_angeldie8};
statetype s_angeldie8	= {false,SPR_ANGEL_DIE7,10,NULL,NULL,&s_angeldie9};
statetype s_angeldie9	= {false,SPR_ANGEL_DEAD,130,NULL,A_Victory,&s_angeldie9};

statetype s_angelshoot1 	= {false,SPR_ANGEL_SHOOT1,10,NULL,A_StartAttack,&s_angelshoot2};
statetype s_angelshoot2 	= {false,SPR_ANGEL_SHOOT2,20,NULL,T_Launch,&s_angelshoot3};
statetype s_angelshoot3 	= {false,SPR_ANGEL_SHOOT1,10,NULL,A_Relaunch,&s_angelshoot2};

statetype s_angeltired 	= {false,SPR_ANGEL_TIRED1,40,NULL,A_Breathing,&s_angeltired2};
statetype s_angeltired2	= {false,SPR_ANGEL_TIRED2,40,NULL,NULL,&s_angeltired3};
statetype s_angeltired3	= {false,SPR_ANGEL_TIRED1,40,NULL,A_Breathing,&s_angeltired4};
statetype s_angeltired4	= {false,SPR_ANGEL_TIRED2,40,NULL,NULL,&s_angeltired5};
statetype s_angeltired5	= {false,SPR_ANGEL_TIRED1,40,NULL,A_Breathing,&s_angeltired6};
statetype s_angeltired6	= {false,SPR_ANGEL_TIRED2,40,NULL,NULL,&s_angeltired7};
statetype s_angeltired7	= {false,SPR_ANGEL_TIRED1,40,NULL,A_Breathing,&s_angelchase1};

statetype s_spark1 	= {false,SPR_SPARK1,6,T_Projectile,NULL,&s_spark2};
statetype s_spark2 	= {false,SPR_SPARK2,6,T_Projectile,NULL,&s_spark3};
statetype s_spark3 	= {false,SPR_SPARK3,6,T_Projectile,NULL,&s_spark4};
statetype s_spark4 	= {false,SPR_SPARK4,6,T_Projectile,NULL,&s_spark1};


#pragma argsused
void A_Slurpie (objtype *ob)
{
 SD_PlaySound(SLURPIESND);
}

#pragma argsused
void A_Breathing (objtype *ob)
{
 SD_PlaySound(ANGELTIREDSND);
}

/*
===============
=
= SpawnAngel
=
===============
*/

void SpawnAngel (int16_t tilex, int16_t tiley)
{
	uint16_t	far *map,tile;


	if (SoundBlasterPresent && DigiMode != sds_Off)
		s_angeldie11.tictime = 105;

	SpawnNewObj (tilex,tiley,&s_angelstand);
	new->obclass = angelobj;
	new->hitpoints = starthitpoints[gamestate.difficulty][en_angel];
	new->flags |= FL_SHOOTABLE|FL_AMBUSH;
	if (!loadedgame)
	  gamestate.killtotal++;
}


/*
=================
=
= A_Victory
=
=================
*/

#pragma argsused
void A_Victory (objtype *ob)
{
	playstate = ex_victorious;
}


/*
=================
=
= A_StartAttack
=
=================
*/

void A_StartAttack (objtype *ob)
{
	ob->temp1 = 0;
}


/*
=================
=
= A_Relaunch
=
=================
*/

void A_Relaunch (objtype *ob)
{
	if (++ob->temp1 == 3)
	{
		NewState (ob,&s_angeltired);
		return;
	}

	if (US_RndT()&1)
	{
		NewState (ob,&s_angelchase1);
		return;
	}
}




//
// spectre
//
void T_SpectreWait (objtype *ob);
void A_Dormant (objtype *ob);

statetype s_spectrewait1	= {false,SPR_SPECTRE_W1,10,T_Stand,NULL,&s_spectrewait2};
statetype s_spectrewait2	= {false,SPR_SPECTRE_W2,10,T_Stand,NULL,&s_spectrewait3};
statetype s_spectrewait3	= {false,SPR_SPECTRE_W3,10,T_Stand,NULL,&s_spectrewait4};
statetype s_spectrewait4	= {false,SPR_SPECTRE_W4,10,T_Stand,NULL,&s_spectrewait1};

statetype s_spectrechase1	= {false,SPR_SPECTRE_W1,10,T_Ghosts,NULL,&s_spectrechase2};
statetype s_spectrechase2	= {false,SPR_SPECTRE_W2,10,T_Ghosts,NULL,&s_spectrechase3};
statetype s_spectrechase3	= {false,SPR_SPECTRE_W3,10,T_Ghosts,NULL,&s_spectrechase4};
statetype s_spectrechase4	= {false,SPR_SPECTRE_W4,10,T_Ghosts,NULL,&s_spectrechase1};

statetype s_spectredie1	= {false,SPR_SPECTRE_F1,10,NULL,NULL,&s_spectredie2};
statetype s_spectredie2	= {false,SPR_SPECTRE_F2,10,NULL,NULL,&s_spectredie3};
statetype s_spectredie3	= {false,SPR_SPECTRE_F3,10,NULL,NULL,&s_spectredie4};
statetype s_spectredie4	= {false,SPR_SPECTRE_F4,300,NULL,NULL,&s_spectrewake};
statetype s_spectrewake	= {false,SPR_SPECTRE_F4,10,NULL,A_Dormant,&s_spectrewake};

/*
===============
=
= SpawnSpectre
=
===============
*/

void SpawnSpectre (int16_t tilex, int16_t tiley)
{
	uint16_t	far *map,tile;

	SpawnNewObj (tilex,tiley,&s_spectrewait1);
	new->obclass = spectreobj;
	new->hitpoints = starthitpoints[gamestate.difficulty][en_spectre];
	new->flags |= FL_SHOOTABLE|FL_AMBUSH; // |FL_NEVERMARK|FL_NONMARK;
	if (!loadedgame)
	  gamestate.killtotal++;
}


/*
===============
=
= A_Dormant
=
===============
*/

void A_Dormant (objtype *ob)
{
	int32_t	deltax,deltay;
	int16_t	xl,xh,yl,yh;
	int16_t	x,y;
	uint16_t	tile;

	deltax = ob->x - player->x;
	if (deltax < -MINACTORDIST || deltax > MINACTORDIST)
		goto moveok;
	deltay = ob->y - player->y;
	if (deltay < -MINACTORDIST || deltay > MINACTORDIST)
		goto moveok;

	return;
moveok:

	xl = (ob->x-MINDIST) >> TILESHIFT;
	xh = (ob->x+MINDIST) >> TILESHIFT;
	yl = (ob->y-MINDIST) >> TILESHIFT;
	yh = (ob->y+MINDIST) >> TILESHIFT;

	for (y=yl ; y<=yh ; y++)
		for (x=xl ; x<=xh ; x++)
		{
			tile = actorat[x][y];
			if (!tile)
				continue;
			if (tile<256)
				return;
			if (((objtype *)tile)->flags&FL_SHOOTABLE)
				return;
		}

	ob->flags |= FL_AMBUSH | FL_SHOOTABLE;
	ob->flags &= ~FL_ATTACKMODE;
	ob->dir = nodir;
	NewState (ob,&s_spectrewait1);
}


#endif

/*
=============================================================================

						 SCHABBS / GIFT / FAT

=============================================================================
*/

#ifndef SPEAR
/*
===============
=
= SpawnGhosts
=
===============
*/

void SpawnGhosts (int16_t which, int16_t tilex, int16_t tiley)
{
	switch(which)
	{
	 case en_blinky:
	   SpawnNewObj (tilex,tiley,s_blinkychase1);
	   break;
	 case en_clyde:
	   SpawnNewObj (tilex,tiley,s_clydechase1);
	   break;
	 case en_pinky:
	   SpawnNewObj (tilex,tiley,s_pinkychase1);
	   break;
	 case en_inky:
	   SpawnNewObj (tilex,tiley,s_inkychase1);
	   break;
	}

	new->obclass = ghostobj;
	new->speed = SPDDOG;

	new->dir = east;
	new->flags |= FL_AMBUSH;
	if (!loadedgame)
	  gamestate.killtotal++;
}



void	T_Gift (objtype *ob);
void	T_GiftThrow (objtype *ob);

void	T_Fat (objtype *ob);
void	T_FatThrow (objtype *ob);

//
// schabb
//
void Act2SchabbMakeStates()
{
    MAKESTATE(s_schabbstand,false,SPR_SCHABB_W1,0,T_Stand,NULL,s_schabbstand);

    MAKESTATE(s_schabbchase1,false,SPR_SCHABB_W1,10,T_Schabb,NULL,s_schabbchase1s);
    MAKESTATE(s_schabbchase1s,false,SPR_SCHABB_W1,3,NULL,NULL,s_schabbchase2);
    MAKESTATE(s_schabbchase2,false,SPR_SCHABB_W2,8,T_Schabb,NULL,s_schabbchase3);
    MAKESTATE(s_schabbchase3,false,SPR_SCHABB_W3,10,T_Schabb,NULL,s_schabbchase3s);
    MAKESTATE(s_schabbchase3s,false,SPR_SCHABB_W3,3,NULL,NULL,s_schabbchase4);
    MAKESTATE(s_schabbchase4,false,SPR_SCHABB_W4,8,T_Schabb,NULL,s_schabbchase1);

    MAKESTATE(s_schabbdeathcam,false,SPR_SCHABB_W1,1,NULL,NULL,s_schabbdie1);

    MAKESTATE(s_schabbdie1,false,SPR_SCHABB_W1,10,NULL,A_DeathScream,s_schabbdie2);
    MAKESTATE(s_schabbdie2,false,SPR_SCHABB_W1,10,NULL,NULL,s_schabbdie3);
    MAKESTATE(s_schabbdie3,false,SPR_SCHABB_DIE1,10,NULL,NULL,s_schabbdie4);
    MAKESTATE(s_schabbdie4,false,SPR_SCHABB_DIE2,10,NULL,NULL,s_schabbdie5);
    MAKESTATE(s_schabbdie5,false,SPR_SCHABB_DIE3,10,NULL,NULL,s_schabbdie6);
    MAKESTATE(s_schabbdie6,false,SPR_SCHABB_DEAD,20,NULL,A_StartDeathCam,s_schabbdie6);

    MAKESTATE(s_schabbshoot1,false,SPR_SCHABB_SHOOT1,30,NULL,NULL,s_schabbshoot2);
    MAKESTATE(s_schabbshoot2,false,SPR_SCHABB_SHOOT2,10,NULL,T_SchabbThrow,s_schabbchase1);

    MAKESTATE(s_needle1,false,SPR_HYPO1,6,T_Projectile,NULL,s_needle2);
    MAKESTATE(s_needle2,false,SPR_HYPO2,6,T_Projectile,NULL,s_needle3);
    MAKESTATE(s_needle3,false,SPR_HYPO3,6,T_Projectile,NULL,s_needle4);
    MAKESTATE(s_needle4,false,SPR_HYPO4,6,T_Projectile,NULL,s_needle1);
}

//
// gift
//
void Act2GiftMakeStates()
{
    MAKESTATE(s_giftstand,false,SPR_GIFT_W1,0,T_Stand,NULL,s_giftstand);

    MAKESTATE(s_giftchase1,false,SPR_GIFT_W1,10,T_Gift,NULL,s_giftchase1s);
    MAKESTATE(s_giftchase1s,false,SPR_GIFT_W1,3,NULL,NULL,s_giftchase2);
    MAKESTATE(s_giftchase2,false,SPR_GIFT_W2,8,T_Gift,NULL,s_giftchase3);
    MAKESTATE(s_giftchase3,false,SPR_GIFT_W3,10,T_Gift,NULL,s_giftchase3s);
    MAKESTATE(s_giftchase3s,false,SPR_GIFT_W3,3,NULL,NULL,s_giftchase4);
    MAKESTATE(s_giftchase4,false,SPR_GIFT_W4,8,T_Gift,NULL,s_giftchase1);

    MAKESTATE(s_giftdeathcam,false,SPR_GIFT_W1,1,NULL,NULL,s_giftdie1);

    MAKESTATE(s_giftdie1,false,SPR_GIFT_W1,1,NULL,A_DeathScream,s_giftdie2);
    MAKESTATE(s_giftdie2,false,SPR_GIFT_W1,10,NULL,NULL,s_giftdie3);
    MAKESTATE(s_giftdie3,false,SPR_GIFT_DIE1,10,NULL,NULL,s_giftdie4);
    MAKESTATE(s_giftdie4,false,SPR_GIFT_DIE2,10,NULL,NULL,s_giftdie5);
    MAKESTATE(s_giftdie5,false,SPR_GIFT_DIE3,10,NULL,NULL,s_giftdie6);
    MAKESTATE(s_giftdie6,false,SPR_GIFT_DEAD,20,NULL,A_StartDeathCam,s_giftdie6);

    MAKESTATE(s_giftshoot1,false,SPR_GIFT_SHOOT1,30,NULL,NULL,s_giftshoot2);
    MAKESTATE(s_giftshoot2,false,SPR_GIFT_SHOOT2,10,NULL,T_GiftThrow,s_giftchase1);
}

//
// fat
//
void Act2FatMakeStates()
{
    MAKESTATE(s_fatstand,false,SPR_FAT_W1,0,T_Stand,NULL,s_fatstand);

    MAKESTATE(s_fatchase1,false,SPR_FAT_W1,10,T_Fat,NULL,s_fatchase1s);
    MAKESTATE(s_fatchase1s,false,SPR_FAT_W1,3,NULL,NULL,s_fatchase2);
    MAKESTATE(s_fatchase2,false,SPR_FAT_W2,8,T_Fat,NULL,s_fatchase3);
    MAKESTATE(s_fatchase3,false,SPR_FAT_W3,10,T_Fat,NULL,s_fatchase3s);
    MAKESTATE(s_fatchase3s,false,SPR_FAT_W3,3,NULL,NULL,s_fatchase4);
    MAKESTATE(s_fatchase4,false,SPR_FAT_W4,8,T_Fat,NULL,s_fatchase1);

    MAKESTATE(s_fatdeathcam,false,SPR_FAT_W1,1,NULL,NULL,s_fatdie1);

    MAKESTATE(s_fatdie1,false,SPR_FAT_W1,1,NULL,A_DeathScream,s_fatdie2);
    MAKESTATE(s_fatdie2,false,SPR_FAT_W1,10,NULL,NULL,s_fatdie3);
    MAKESTATE(s_fatdie3,false,SPR_FAT_DIE1,10,NULL,NULL,s_fatdie4);
    MAKESTATE(s_fatdie4,false,SPR_FAT_DIE2,10,NULL,NULL,s_fatdie5);
    MAKESTATE(s_fatdie5,false,SPR_FAT_DIE3,10,NULL,NULL,s_fatdie6);
    MAKESTATE(s_fatdie6,false,SPR_FAT_DEAD,20,NULL,A_StartDeathCam,s_fatdie6);

    MAKESTATE(s_fatshoot1,false,SPR_FAT_SHOOT1,30,NULL,NULL,s_fatshoot2);
    MAKESTATE(s_fatshoot2,false,SPR_FAT_SHOOT2,10,NULL,T_GiftThrow,s_fatshoot3);
    MAKESTATE(s_fatshoot3,false,SPR_FAT_SHOOT3,10,NULL,T_Shoot,s_fatshoot4);
    MAKESTATE(s_fatshoot4,false,SPR_FAT_SHOOT4,10,NULL,T_Shoot,s_fatshoot5);
    MAKESTATE(s_fatshoot5,false,SPR_FAT_SHOOT3,10,NULL,T_Shoot,s_fatshoot6);
    MAKESTATE(s_fatshoot6,false,SPR_FAT_SHOOT4,10,NULL,T_Shoot,s_fatchase1);
}

/*
===============
=
= SpawnSchabbs
=
===============
*/

void SpawnSchabbs (int16_t tilex, int16_t tiley)
{
	if (DigiMode != sds_Off)
		GETSTATE(s_schabbdie2).tictime = 140;
	else
        GETSTATE(s_schabbdie2).tictime = 5;

	SpawnNewObj (tilex,tiley,s_schabbstand);
	new->speed = SPDPATROL;

	new->obclass = schabbobj;
	new->hitpoints = starthitpoints[gamestate.difficulty][en_schabbs];
	new->dir = south;
	new->flags |= FL_SHOOTABLE|FL_AMBUSH;
	if (!loadedgame)
	  gamestate.killtotal++;
}


/*
===============
=
= SpawnGift
=
===============
*/

void SpawnGift (int16_t tilex, int16_t tiley)
{
	if (DigiMode != sds_Off)
	  GETSTATE(s_giftdie2).tictime = 140;
	else
	  GETSTATE(s_giftdie2).tictime = 5;

	SpawnNewObj (tilex,tiley,s_giftstand);
	new->speed = SPDPATROL;

	new->obclass = giftobj;
	new->hitpoints = starthitpoints[gamestate.difficulty][en_gift];
	new->dir = north;
	new->flags |= FL_SHOOTABLE|FL_AMBUSH;
	if (!loadedgame)
	  gamestate.killtotal++;
}


/*
===============
=
= SpawnFat
=
===============
*/

void SpawnFat (int16_t tilex, int16_t tiley)
{
	if (DigiMode != sds_Off)
	  GETSTATE(s_fatdie2).tictime = 140;
	else
	  GETSTATE(s_fatdie2).tictime = 5;

	SpawnNewObj (tilex,tiley,s_fatstand);
	new->speed = SPDPATROL;

	new->obclass = fatobj;
	new->hitpoints = starthitpoints[gamestate.difficulty][en_fat];
	new->dir = south;
	new->flags |= FL_SHOOTABLE|FL_AMBUSH;
	if (!loadedgame)
	  gamestate.killtotal++;
}


/*
=================
=
= T_SchabbThrow
=
=================
*/

void T_SchabbThrow (objtype *ob)
{
	int32_t	deltax,deltay;
	float	angle;
	int16_t	iangle;

	deltax = player->x - ob->x;
	deltay = ob->y - player->y;
	angle = atan2 (deltay,deltax);
	if (angle<0)
		angle = M_PI*2+angle;
	iangle = (int16_t)(angle/(M_PI*2)*ANGLES);

	GetNewActor ();
	new->state = s_needle1;
	new->ticcount = 1;

	new->tilex = ob->tilex;
	new->tiley = ob->tiley;
	new->x = ob->x;
	new->y = ob->y;
	new->obclass = needleobj;
	new->dir = nodir;
	new->angle = iangle;
	new->speed = 0x2000l;

	new->flags = FL_NONMARK;
	new->active = ac_yes;

	PlaySoundLocActor (SCHABBSTHROWSND,new);
}

/*
=================
=
= T_GiftThrow
=
=================
*/

void T_GiftThrow (objtype *ob)
{
	int32_t	deltax,deltay;
	float	angle;
	int16_t	iangle;

	deltax = player->x - ob->x;
	deltay = ob->y - player->y;
	angle = atan2 (deltay,deltax);
	if (angle<0)
		angle = M_PI*2+angle;
	iangle = (int16_t)(angle/(M_PI*2)*ANGLES);

	GetNewActor ();
	new->state = s_rocket;
	new->ticcount = 1;

	new->tilex = ob->tilex;
	new->tiley = ob->tiley;
	new->x = ob->x;
	new->y = ob->y;
	new->obclass = rocketobj;
	new->dir = nodir;
	new->angle = iangle;
	new->speed = 0x2000l;
	new->flags = FL_NONMARK;
	new->active = ac_yes;

	PlaySoundLocActor (MISSILEFIRESND,new);
}



/*
=================
=
= T_Schabb
=
=================
*/

void T_Schabb (objtype *ob)
{
	int32_t move;
	int16_t	dx,dy,dist;
	boolean	dodge;

	dodge = false;
	dx = abs(ob->tilex - player->tilex);
	dy = abs(ob->tiley - player->tiley);
	dist = dx>dy ? dx : dy;

	if (CheckLine(ob))						// got a shot at player?
	{

		if ( US_RndT() < (tics<<3) )
		{
		//
		// go into attack frame
		//
			NewState (ob,s_schabbshoot1);
			return;
		}
		dodge = true;
	}

	if (ob->dir == nodir)
	{
		if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);
		if (ob->dir == nodir)
			return;							// object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (ob->distance < 0)
		{
		//
		// waiting for a door to open
		//
			OpenDoor (-ob->distance-1);
			if (doorobjlist[-ob->distance-1].action != dr_open)
				return;
			ob->distance = TILEGLOBAL;	// go ahead, the door is now opoen
		}

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		if (dist <4)
			SelectRunDir (ob);
		else if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);

		if (ob->dir == nodir)
			return;							// object is blocked in
	}

}




/*
=================
=
= T_Gift
=
=================
*/

void T_Gift (objtype *ob)
{
	int32_t move;
	int16_t	dx,dy,dist;
	boolean	dodge;

	dodge = false;
	dx = abs(ob->tilex - player->tilex);
	dy = abs(ob->tiley - player->tiley);
	dist = dx>dy ? dx : dy;

	if (CheckLine(ob))						// got a shot at player?
	{

		if ( US_RndT() < (tics<<3) )
		{
		//
		// go into attack frame
		//
			NewState (ob,s_giftshoot1);
			return;
		}
		dodge = true;
	}

	if (ob->dir == nodir)
	{
		if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);
		if (ob->dir == nodir)
			return;							// object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (ob->distance < 0)
		{
		//
		// waiting for a door to open
		//
			OpenDoor (-ob->distance-1);
			if (doorobjlist[-ob->distance-1].action != dr_open)
				return;
			ob->distance = TILEGLOBAL;	// go ahead, the door is now opoen
		}

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		if (dist <4)
			SelectRunDir (ob);
		else if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);

		if (ob->dir == nodir)
			return;							// object is blocked in
	}

}




/*
=================
=
= T_Fat
=
=================
*/

void T_Fat (objtype *ob)
{
	int32_t move;
	int16_t	dx,dy,dist;
	boolean	dodge;

	dodge = false;
	dx = abs(ob->tilex - player->tilex);
	dy = abs(ob->tiley - player->tiley);
	dist = dx>dy ? dx : dy;

	if (CheckLine(ob))						// got a shot at player?
	{

		if ( US_RndT() < (tics<<3) )
		{
		//
		// go into attack frame
		//
			NewState (ob,s_fatshoot1);
			return;
		}
		dodge = true;
	}

	if (ob->dir == nodir)
	{
		if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);
		if (ob->dir == nodir)
			return;							// object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (ob->distance < 0)
		{
		//
		// waiting for a door to open
		//
			OpenDoor (-ob->distance-1);
			if (doorobjlist[-ob->distance-1].action != dr_open)
				return;
			ob->distance = TILEGLOBAL;	// go ahead, the door is now opoen
		}

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		if (dist <4)
			SelectRunDir (ob);
		else if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);

		if (ob->dir == nodir)
			return;							// object is blocked in
	}

}



/*
=============================================================================

							HITLERS

=============================================================================
*/


//
// fake
//
void Act2FakeMakeStates()
{
    MAKESTATE(s_fakestand,false,SPR_FAKE_W1,0,T_Stand,NULL,s_fakestand);

    MAKESTATE(s_fakechase1,false,SPR_FAKE_W1,10,T_Fake,NULL,s_fakechase1s);
    MAKESTATE(s_fakechase1s,false,SPR_FAKE_W1,3,NULL,NULL,s_fakechase2);
    MAKESTATE(s_fakechase2,false,SPR_FAKE_W2,8,T_Fake,NULL,s_fakechase3);
    MAKESTATE(s_fakechase3,false,SPR_FAKE_W3,10,T_Fake,NULL,s_fakechase3s);
    MAKESTATE(s_fakechase3s,false,SPR_FAKE_W3,3,NULL,NULL,s_fakechase4);
    MAKESTATE(s_fakechase4,false,SPR_FAKE_W4,8,T_Fake,NULL,s_fakechase1);

    MAKESTATE(s_fakedie1,false,SPR_FAKE_DIE1,10,NULL,A_DeathScream,s_fakedie2);
    MAKESTATE(s_fakedie2,false,SPR_FAKE_DIE2,10,NULL,NULL,s_fakedie3);
    MAKESTATE(s_fakedie3,false,SPR_FAKE_DIE3,10,NULL,NULL,s_fakedie4);
    MAKESTATE(s_fakedie4,false,SPR_FAKE_DIE4,10,NULL,NULL,s_fakedie5);
    MAKESTATE(s_fakedie5,false,SPR_FAKE_DIE5,10,NULL,NULL,s_fakedie6);
    MAKESTATE(s_fakedie6,false,SPR_FAKE_DEAD,0,NULL,NULL,s_fakedie6);

    MAKESTATE(s_fakeshoot1,false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,s_fakeshoot2);
    MAKESTATE(s_fakeshoot2,false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,s_fakeshoot3);
    MAKESTATE(s_fakeshoot3,false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,s_fakeshoot4);
    MAKESTATE(s_fakeshoot4,false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,s_fakeshoot5);
    MAKESTATE(s_fakeshoot5,false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,s_fakeshoot6);
    MAKESTATE(s_fakeshoot6,false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,s_fakeshoot7);
    MAKESTATE(s_fakeshoot7,false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,s_fakeshoot8);
    MAKESTATE(s_fakeshoot8,false,SPR_FAKE_SHOOT,8,NULL,T_FakeFire,s_fakeshoot9);
    MAKESTATE(s_fakeshoot9,false,SPR_FAKE_SHOOT,8,NULL,NULL,s_fakechase1);

    MAKESTATE(s_fire1,false,SPR_FIRE1,6,NULL,T_Projectile,s_fire2);
    MAKESTATE(s_fire2,false,SPR_FIRE2,6,NULL,T_Projectile,s_fire1);
}

//
// hitler
//
void Act2HitlerMakeStates()
{
    MAKESTATE(s_mechastand,false,SPR_MECHA_W1,0,T_Stand,NULL,s_mechastand);

    MAKESTATE(s_mechachase1,false,SPR_MECHA_W1,10,T_Chase,A_MechaSound,s_mechachase1s);
    MAKESTATE(s_mechachase1s,false,SPR_MECHA_W1,6,NULL,NULL,s_mechachase2);
    MAKESTATE(s_mechachase2,false,SPR_MECHA_W2,8,T_Chase,NULL,s_mechachase3);
    MAKESTATE(s_mechachase3,false,SPR_MECHA_W3,10,T_Chase,A_MechaSound,s_mechachase3s);
    MAKESTATE(s_mechachase3s,false,SPR_MECHA_W3,6,NULL,NULL,s_mechachase4);
    MAKESTATE(s_mechachase4,false,SPR_MECHA_W4,8,T_Chase,NULL,s_mechachase1);

    MAKESTATE(s_mechadie1,false,SPR_MECHA_DIE1,10,NULL,A_DeathScream,s_mechadie2);
    MAKESTATE(s_mechadie2,false,SPR_MECHA_DIE2,10,NULL,NULL,s_mechadie3);
    MAKESTATE(s_mechadie3,false,SPR_MECHA_DIE3,10,NULL,A_HitlerMorph,s_mechadie4);
    MAKESTATE(s_mechadie4,false,SPR_MECHA_DEAD,0,NULL,NULL,s_mechadie4);

    MAKESTATE(s_mechashoot1,false,SPR_MECHA_SHOOT1,30,NULL,NULL,s_mechashoot2);
    MAKESTATE(s_mechashoot2,false,SPR_MECHA_SHOOT2,10,NULL,T_Shoot,s_mechashoot3);
    MAKESTATE(s_mechashoot3,false,SPR_MECHA_SHOOT3,10,NULL,T_Shoot,s_mechashoot4);
    MAKESTATE(s_mechashoot4,false,SPR_MECHA_SHOOT2,10,NULL,T_Shoot,s_mechashoot5);
    MAKESTATE(s_mechashoot5,false,SPR_MECHA_SHOOT3,10,NULL,T_Shoot,s_mechashoot6);
    MAKESTATE(s_mechashoot6,false,SPR_MECHA_SHOOT2,10,NULL,T_Shoot,s_mechachase1);


    MAKESTATE(s_hitlerchase1,false,SPR_HITLER_W1,6,T_Chase,NULL,s_hitlerchase1s);
    MAKESTATE(s_hitlerchase1s,false,SPR_HITLER_W1,4,NULL,NULL,s_hitlerchase2);
    MAKESTATE(s_hitlerchase2,false,SPR_HITLER_W2,2,T_Chase,NULL,s_hitlerchase3);
    MAKESTATE(s_hitlerchase3,false,SPR_HITLER_W3,6,T_Chase,NULL,s_hitlerchase3s);
    MAKESTATE(s_hitlerchase3s,false,SPR_HITLER_W3,4,NULL,NULL,s_hitlerchase4);
    MAKESTATE(s_hitlerchase4,false,SPR_HITLER_W4,2,T_Chase,NULL,s_hitlerchase1);

    MAKESTATE(s_hitlerdeathcam,false,SPR_HITLER_W1,10,NULL,NULL,s_hitlerdie1);

    MAKESTATE(s_hitlerdie1,false,SPR_HITLER_W1,1,NULL,A_DeathScream,s_hitlerdie2);
    MAKESTATE(s_hitlerdie2,false,SPR_HITLER_W1,10,NULL,NULL,s_hitlerdie3);
    MAKESTATE(s_hitlerdie3,false,SPR_HITLER_DIE1,10,NULL,A_Slurpie,s_hitlerdie4);
    MAKESTATE(s_hitlerdie4,false,SPR_HITLER_DIE2,10,NULL,NULL,s_hitlerdie5);
    MAKESTATE(s_hitlerdie5,false,SPR_HITLER_DIE3,10,NULL,NULL,s_hitlerdie6);
    MAKESTATE(s_hitlerdie6,false,SPR_HITLER_DIE4,10,NULL,NULL,s_hitlerdie7);
    MAKESTATE(s_hitlerdie7,false,SPR_HITLER_DIE5,10,NULL,NULL,s_hitlerdie8);
    MAKESTATE(s_hitlerdie8,false,SPR_HITLER_DIE6,10,NULL,NULL,s_hitlerdie9);
    MAKESTATE(s_hitlerdie9,false,SPR_HITLER_DIE7,10,NULL,NULL,s_hitlerdie10);
    MAKESTATE(s_hitlerdie10,false,SPR_HITLER_DEAD,20,NULL,A_StartDeathCam,s_hitlerdie10);

    MAKESTATE(s_hitlershoot1,false,SPR_HITLER_SHOOT1,30,NULL,NULL,s_hitlershoot2);
    MAKESTATE(s_hitlershoot2,false,SPR_HITLER_SHOOT2,10,NULL,T_Shoot,s_hitlershoot3);
    MAKESTATE(s_hitlershoot3,false,SPR_HITLER_SHOOT3,10,NULL,T_Shoot,s_hitlershoot4);
    MAKESTATE(s_hitlershoot4,false,SPR_HITLER_SHOOT2,10,NULL,T_Shoot,s_hitlershoot5);
    MAKESTATE(s_hitlershoot5,false,SPR_HITLER_SHOOT3,10,NULL,T_Shoot,s_hitlershoot6);
    MAKESTATE(s_hitlershoot6,false,SPR_HITLER_SHOOT2,10,NULL,T_Shoot,s_hitlerchase1);
}


/*
===============
=
= SpawnFakeHitler
=
===============
*/

void SpawnFakeHitler (int16_t tilex, int16_t tiley)
{
	if (DigiMode != sds_Off)
	  GETSTATE(s_hitlerdie2).tictime = 140;
	else
	  GETSTATE(s_hitlerdie2).tictime = 5;

	SpawnNewObj (tilex,tiley,s_fakestand);
	new->speed = SPDPATROL;

	new->obclass = fakeobj;
	new->hitpoints = starthitpoints[gamestate.difficulty][en_fake];
	new->dir = north;
	new->flags |= FL_SHOOTABLE|FL_AMBUSH;
	if (!loadedgame)
	  gamestate.killtotal++;
}


/*
===============
=
= SpawnHitler
=
===============
*/

void SpawnHitler (int16_t tilex, int16_t tiley)
{
	if (DigiMode != sds_Off)
		GETSTATE(s_hitlerdie2).tictime = 140;
	else
		GETSTATE(s_hitlerdie2).tictime = 5;


	SpawnNewObj (tilex,tiley,s_mechastand);
	new->speed = SPDPATROL;

	new->obclass = mechahitlerobj;
	new->hitpoints = starthitpoints[gamestate.difficulty][en_hitler];
	new->dir = south;
	new->flags |= FL_SHOOTABLE|FL_AMBUSH;
	if (!loadedgame)
	  gamestate.killtotal++;
}


/*
===============
=
= A_HitlerMorph
=
===============
*/

void A_HitlerMorph (objtype *ob)
{
	uint16_t	hitpoints[4]={500,700,800,900};


	SpawnNewObj (ob->tilex,ob->tiley,s_hitlerchase1);
	new->speed = SPDPATROL*5;

	new->x = ob->x;
	new->y = ob->y;

	new->distance = ob->distance;
	new->dir = ob->dir;
	new->flags = ob->flags | FL_SHOOTABLE;

	new->obclass = realhitlerobj;
	new->hitpoints = hitpoints[gamestate.difficulty];
}


////////////////////////////////////////////////////////
//
// A_MechaSound
// A_Slurpie
//
////////////////////////////////////////////////////////
void A_MechaSound (objtype *ob)
{
	if (areabyplayer[ob->areanumber])
		PlaySoundLocActor (MECHSTEPSND,ob);
}


void A_Slurpie (objtype *ob)
{
 SD_PlaySound(SLURPIESND);
}

/*
=================
=
= T_FakeFire
=
=================
*/

void T_FakeFire (objtype *ob)
{
	int32_t	deltax,deltay;
	float	angle;
	int16_t	iangle;

	deltax = player->x - ob->x;
	deltay = ob->y - player->y;
	angle = atan2 (deltay,deltax);
	if (angle<0)
		angle = M_PI*2+angle;
	iangle = (int16_t)(angle/(M_PI*2)*ANGLES);

	GetNewActor ();
	new->state = s_fire1;
	new->ticcount = 1;

	new->tilex = ob->tilex;
	new->tiley = ob->tiley;
	new->x = ob->x;
	new->y = ob->y;
	new->dir = nodir;
	new->angle = iangle;
	new->obclass = fireobj;
	new->speed = 0x1200l;
	new->flags = FL_NEVERMARK;
	new->active = ac_yes;

	PlaySoundLocActor (FLAMETHROWERSND,new);
}



/*
=================
=
= T_Fake
=
=================
*/

void T_Fake (objtype *ob)
{
	int32_t move;

	if (CheckLine(ob))			// got a shot at player?
	{
		if ( US_RndT() < (tics<<1) )
		{
		//
		// go into attack frame
		//
			NewState (ob,s_fakeshoot1);
			return;
		}
	}

	if (ob->dir == nodir)
	{
		SelectDodgeDir (ob);
		if (ob->dir == nodir)
			return;							// object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		SelectDodgeDir (ob);

		if (ob->dir == nodir)
			return;							// object is blocked in
	}

}

#endif
/*
============================================================================

							STAND

============================================================================
*/


/*
===============
=
= T_Stand
=
===============
*/

void T_Stand (objtype *ob)
{
	SightPlayer (ob);
}


/*
============================================================================

								CHASE

============================================================================
*/

/*
=================
=
= T_Chase
=
=================
*/

void T_Chase (objtype *ob)
{
	int32_t move;
	int16_t	dx,dy,dist,chance;
	boolean	dodge;

	if (gamestate.victoryflag)
		return;

	dodge = false;
	if (CheckLine(ob))	// got a shot at player?
	{
		dx = abs(ob->tilex - player->tilex);
		dy = abs(ob->tiley - player->tiley);
		dist = dx>dy ? dx : dy;
		if (!dist || (dist==1 && ob->distance<0x4000) )
			chance = 300;
		else
			chance = (tics<<4)/dist;

		if ( US_RndT()<chance)
		{
		//
		// go into attack frame
		//
			switch (ob->obclass)
			{
			case guardobj:
				NewState (ob,s_grdshoot1);
				break;
			case officerobj:
				NewState (ob,s_ofcshoot1);
				break;
			case mutantobj:
				NewState (ob,s_mutshoot1);
				break;
			case ssobj:
				NewState (ob,s_ssshoot1);
				break;
#ifndef SPEAR
			case bossobj:
				NewState (ob,s_bossshoot1);
				break;
			case gretelobj:
				NewState (ob,s_gretelshoot1);
				break;
			case mechahitlerobj:
				NewState (ob,s_mechashoot1);
				break;
			case realhitlerobj:
				NewState (ob,s_hitlershoot1);
				break;
#else
			case angelobj:
				NewState (ob,s_angelshoot1);
				break;
			case transobj:
				NewState (ob,s_transshoot1);
				break;
			case uberobj:
				NewState (ob,s_ubershoot1);
				break;
			case willobj:
				NewState (ob,s_willshoot1);
				break;
			case deathobj:
				NewState (ob,s_deathshoot1);
				break;
#endif
			}
			return;
		}
		dodge = true;
	}

	if (ob->dir == nodir)
	{
		if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);
		if (ob->dir == nodir)
			return;							// object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (ob->distance < 0)
		{
		//
		// waiting for a door to open
		//
			OpenDoor (-ob->distance-1);
			if (doorobjlist[-ob->distance-1].action != dr_open)
				return;
			ob->distance = TILEGLOBAL;	// go ahead, the door is now opoen
		}

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		if (dodge)
			SelectDodgeDir (ob);
		else
			SelectChaseDir (ob);

		if (ob->dir == nodir)
			return;							// object is blocked in
	}

}


/*
=================
=
= T_Ghosts
=
=================
*/

void T_Ghosts (objtype *ob)
{
	int32_t move;


	if (ob->dir == nodir)
	{
		SelectChaseDir (ob);
		if (ob->dir == nodir)
			return;							// object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		SelectChaseDir (ob);

		if (ob->dir == nodir)
			return;							// object is blocked in
	}

}

/*
=================
=
= T_DogChase
=
=================
*/

void T_DogChase (objtype *ob)
{
	int32_t move;
	int32_t	dx,dy;


	if (ob->dir == nodir)
	{
		SelectDodgeDir (ob);
		if (ob->dir == nodir)
			return;							// object is blocked in
	}

	move = ob->speed*tics;

	while (move)
	{
	//
	// check for byte range
	//
		dx = player->x - ob->x;
		if (dx<0)
			dx = -dx;
		dx -= move;
		if (dx <= MINACTORDIST)
		{
			dy = player->y - ob->y;
			if (dy<0)
				dy = -dy;
			dy -= move;
			if (dy <= MINACTORDIST)
			{
				NewState (ob,s_dogjump1);
				return;
			}
		}

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		//
		// reached goal tile, so select another one
		//

		//
		// fix position to account for round off during moving
		//
		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;

		move -= ob->distance;

		SelectDodgeDir (ob);

		if (ob->dir == nodir)
			return;							// object is blocked in
	}

}



/*
============================================================================

								PATH

============================================================================
*/


/*
===============
=
= SelectPathDir
=
===============
*/

void SelectPathDir (objtype *ob)
{
	uint16_t spot;

	spot = MAPSPOT(ob->tilex,ob->tiley,1)-ICONARROWS;

	if (spot<8)
	{
	// new direction
		ob->dir = spot;
	}

	ob->distance = TILEGLOBAL;

	if (!TryWalk (ob))
		ob->dir = nodir;
}


/*
===============
=
= T_Path
=
===============
*/

void T_Path (objtype *ob)
{
	int32_t 	move;

	if (SightPlayer (ob))
		return;

	if (ob->dir == nodir)
	{
		SelectPathDir (ob);
		if (ob->dir == nodir)
			return;					// all movement is blocked
	}


	move = ob->speed*tics;

	while (move)
	{
		if (ob->distance < 0)
		{
		//
		// waiting for a door to open
		//
			OpenDoor (-ob->distance-1);
			if (doorobjlist[-ob->distance-1].action != dr_open)
				return;
			ob->distance = TILEGLOBAL;	// go ahead, the door is now opoen
		}

		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}

		if (ob->tilex>MAPSIZE || ob->tiley>MAPSIZE)
		{
			sprintf (str,"T_Path hit a wall at %u,%u, dir %u"
			,ob->tilex,ob->tiley,ob->dir);
			Quit (str);
		}



		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;
		move -= ob->distance;

		SelectPathDir (ob);

		if (ob->dir == nodir)
			return;					// all movement is blocked
	}
}


/*
=============================================================================

								FIGHT

=============================================================================
*/


/*
===============
=
= T_Shoot
=
= Try to damage the player, based on skill level and player's speed
=
===============
*/

void T_Shoot (objtype *ob)
{
	int16_t	dx,dy,dist;
	int16_t	hitchance,damage;

	hitchance = 128;

	if (!areabyplayer[ob->areanumber])
		return;

	if (!CheckLine (ob))			// player is behind a wall
	  return;

	dx = abs(ob->tilex - player->tilex);
	dy = abs(ob->tiley - player->tiley);
	dist = dx>dy ? dx:dy;

	if (ob->obclass == ssobj || ob->obclass == bossobj)
		dist = dist*2/3;					// ss are better shots

	if (thrustspeed >= RUNSPEED)
	{
		if (ob->flags&FL_VISABLE)
			hitchance = 160-dist*16;		// player can see to dodge
		else
			hitchance = 160-dist*8;
	}
	else
	{
		if (ob->flags&FL_VISABLE)
			hitchance = 256-dist*16;		// player can see to dodge
		else
			hitchance = 256-dist*8;
	}

// see if the shot was a hit

	if (US_RndT()<hitchance)
	{
		if (dist<2)
			damage = US_RndT()>>2;
		else if (dist<4)
			damage = US_RndT()>>3;
		else
			damage = US_RndT()>>4;

		TakeDamage (damage,ob);
	}

	switch(ob->obclass)
	{
	 case ssobj:
	   PlaySoundLocActor(SSFIRESND,ob);
	   break;
#ifndef SPEAR
	 case giftobj:
	 case fatobj:
	   PlaySoundLocActor(MISSILEFIRESND,ob);
	   break;
	 case mechahitlerobj:
	 case realhitlerobj:
	 case bossobj:
	   PlaySoundLocActor(BOSSFIRESND,ob);
	   break;
	 case schabbobj:
	   PlaySoundLocActor(SCHABBSTHROWSND,ob);
	   break;
	 case fakeobj:
	   PlaySoundLocActor(FLAMETHROWERSND,ob);
	   break;
#endif
	 default:
	   PlaySoundLocActor(NAZIFIRESND,ob);
	}

}


/*
===============
=
= T_Bite
=
===============
*/

void T_Bite (objtype *ob)
{
	int32_t	dx,dy;


	PlaySoundLocActor(DOGATTACKSND,ob);	// JAB

	dx = player->x - ob->x;
	if (dx<0)
		dx = -dx;
	dx -= TILEGLOBAL;
	if (dx <= MINACTORDIST)
	{
		dy = player->y - ob->y;
		if (dy<0)
			dy = -dy;
		dy -= TILEGLOBAL;
		if (dy <= MINACTORDIST)
		{
		   if (US_RndT()<180)
		   {
			   TakeDamage (US_RndT()>>4,ob);
			   return;
		   }
		}
	}

	return;
}


#ifndef SPEAR
/*
============================================================================

							BJ VICTORY

============================================================================
*/


//
// BJ victory
//

void T_BJRun (objtype *ob);
void T_BJJump (objtype *ob);
void T_BJDone (objtype *ob);
void T_BJYell (objtype *ob);

//void T_DeathCam (objtype *ob);

void Act2BJVictoryMakeStates()
{
    MAKESTATE(s_bjrun1,false,SPR_BJ_W1,12,T_BJRun,NULL,s_bjrun1s);
    MAKESTATE(s_bjrun1s,false,SPR_BJ_W1,3, NULL,NULL,s_bjrun2);
    MAKESTATE(s_bjrun2,false,SPR_BJ_W2,8,T_BJRun,NULL,s_bjrun3);
    MAKESTATE(s_bjrun3,false,SPR_BJ_W3,12,T_BJRun,NULL,s_bjrun3s);
    MAKESTATE(s_bjrun3s,false,SPR_BJ_W3,3, NULL,NULL,s_bjrun4);
    MAKESTATE(s_bjrun4,false,SPR_BJ_W4,8,T_BJRun,NULL,s_bjrun1);

    MAKESTATE(s_bjjump1,false,SPR_BJ_JUMP1,14,T_BJJump,NULL,s_bjjump2);
    MAKESTATE(s_bjjump2,false,SPR_BJ_JUMP2,14,T_BJJump,T_BJYell,s_bjjump3);
    MAKESTATE(s_bjjump3,false,SPR_BJ_JUMP3,14,T_BJJump,NULL,s_bjjump4);
    MAKESTATE(s_bjjump4,false,SPR_BJ_JUMP4,300,NULL,T_BJDone,s_bjjump4);

    MAKESTATE(s_deathcam,false,0,0,NULL,NULL,NULLSTATE);
}

/*
===============
=
= SpawnBJVictory
=
===============
*/

void SpawnBJVictory (void)
{
	SpawnNewObj (player->tilex,player->tiley+1,s_bjrun1);
	new->x = player->x;
	new->y = player->y;
	new->obclass = bjobj;
	new->dir = north;
	new->temp1 = 6;			// tiles to run forward
}



/*
===============
=
= T_BJRun
=
===============
*/

void T_BJRun (objtype *ob)
{
	int32_t 	move;

	move = BJRUNSPEED*tics;

	while (move)
	{
		if (move < ob->distance)
		{
			MoveObj (ob,move);
			break;
		}


		ob->x = ((int32_t)ob->tilex<<TILESHIFT)+TILEGLOBAL/2;
		ob->y = ((int32_t)ob->tiley<<TILESHIFT)+TILEGLOBAL/2;
		move -= ob->distance;

		SelectPathDir (ob);

		if ( !(--ob->temp1) )
		{
			NewState (ob,s_bjjump1);
			return;
		}
	}
}


/*
===============
=
= T_BJJump
=
===============
*/

void T_BJJump (objtype *ob)
{
	int32_t 	move;

	move = BJJUMPSPEED*tics;
	MoveObj (ob,move);
}


/*
===============
=
= T_BJYell
=
===============
*/

void T_BJYell (objtype *ob)
{
	PlaySoundLocActor(YEAHSND,ob);	// JAB
}


/*
===============
=
= T_BJDone
=
===============
*/

void T_BJDone (objtype *ob)
{
	playstate = ex_victorious;				// exit castle tile
}



//===========================================================================


/*
===============
=
= CheckPosition
=
===============
*/

boolean	CheckPosition (objtype *ob)
{
	int16_t	x,y,xl,yl,xh,yh;
	uint16_t check;

	xl = (ob->x-PLAYERSIZE) >>TILESHIFT;
	yl = (ob->y-PLAYERSIZE) >>TILESHIFT;

	xh = (ob->x+PLAYERSIZE) >>TILESHIFT;
	yh = (ob->y+PLAYERSIZE) >>TILESHIFT;

	//
	// check for solid walls
	//
	for (y=yl;y<=yh;y++)
		for (x=xl;x<=xh;x++)
		{
			check = actorat[x][y];
			if (check && check<256)
				return false;
		}

	return true;
}


/*
===============
=
= A_StartDeathCam
=
===============
*/

void	A_StartDeathCam (objtype *ob)
{
	int32_t	dx,dy;
	float	fangle;
	int32_t xmove,ymove;
	int32_t	dist;
	int16_t	temp,i;

	FinishPaletteShifts ();

	VW_WaitVBL (100);

	if (gamestate.victoryflag)
	{
		playstate = ex_victorious;				// exit castle tile
		return;
	}

	gamestate.victoryflag = true;
	VW_Bar (0,0,320,200-STATUSLINES,127);
	FizzleFade(bufferofs,displayofs,320,200-STATUSLINES,70,false);

	PM_UnlockMainMem ();
	CA_UpLevel ();
	CacheLump(LEVELEND_LUMP_START,LEVELEND_LUMP_END);
	#ifdef JAPAN
	#ifndef JAPDEMO
	CA_CacheScreen(C_LETSSEEPIC);
	#endif
	#else
	Write(0,7,STR_SEEAGAIN);
	#endif
	CA_DownLevel ();
	PM_CheckMainMem ();

	VW_UpdateScreen ();

	IN_UserInput(300);

//
// line angle up exactly
//
	NewState (player,s_deathcam);

	player->x = gamestate.killx;
	player->y = gamestate.killy;

	dx = ob->x - player->x;
	dy = player->y - ob->y;

	fangle = atan2(dy,dx);			// returns -pi to pi
	if (fangle<0)
		fangle = M_PI*2+fangle;

	player->angle = (int16_t)(fangle/(M_PI*2)*ANGLES);

//
// try to position as close as possible without being in a wall
//
	dist = 0x14000l;
	do
	{
		xmove = FixedByFrac(dist,costable[player->angle]);
		ymove = -FixedByFrac(dist,sintable[player->angle]);

		player->x = ob->x - xmove;
		player->y = ob->y - ymove;
		dist += 0x1000;

	} while (!CheckPosition (player));
	plux = player->x >> UNSIGNEDSHIFT;			// scale to fit in unsigned
	pluy = player->y >> UNSIGNEDSHIFT;
	player->tilex = player->x >> TILESHIFT;		// scale to tile values
	player->tiley = player->y >> TILESHIFT;

//
// go back to the game
//
	temp = bufferofs;
	for (i=0;i<3;i++)
	{
		bufferofs = screenloc[i];
		DrawPlayBorder ();
	}
	bufferofs = temp;

	fizzlein = true;
	switch (ob->obclass)
	{
#ifndef SPEAR
	case schabbobj:
		NewState (ob,s_schabbdeathcam);
		break;
	case realhitlerobj:
		NewState (ob,s_hitlerdeathcam);
		break;
	case giftobj:
		NewState (ob,s_giftdeathcam);
		break;
	case fatobj:
		NewState (ob,s_fatdeathcam);
		break;
#endif
	}

}

#endif
