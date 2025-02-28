/****** eigen_stuff_linalg.c  ******************/
/* Linear algebra for eigenvalue and eigevector computation routines.
   These routines work with color vectors

* K.O. 8/99 Started. 
* UH did some stuff to this I think
* EBG 2/15/02 changed phi -> phi1, mass -> mass1 so could be used 
*    w/ ks_imp_dyn2
* EBG 6/2004 fixed some memory leaks
* MIMD version 7
*/

/* This routine is the routine that applies the Matrix, whose eigenvalues    *
 * we want to compute, to a vector. For this specific application it is the  *
 * -D_slash^2 of the KS fermions. We only compute on the "parity" sites.     *
 * Where parity can be EVEN, ODD, or ENENANDODD                              */

#include "generic_ks_includes.h"
#include "../include/dslash_ks_redefine.h"
#include "../include/flavor_ops.h"

/************************************************************************/
/* Temporary su3_vectors used for the squaring */ 
static su3_vector *temp = NULL ;

/************************************************************************/


#ifndef USE_DSLASH_SPECIAL
/* The Matrix_Vec_mult and cleanup_Matrix() WITHOUT using dslash_special */
/************************************************************************/

void Matrix_Vec_mult(su3_vector *src, su3_vector *res, int parity,
		     imp_ferm_links_t *fn )
{  
  register site *s;
  register  int i;
  int otherparity = EVENANDODD;

  if(temp == NULL ){
    temp = (su3_vector *)malloc(sites_on_node*sizeof(su3_vector));
  }
  
  switch(parity){
  case EVEN:
    otherparity = ODD ;
    break ;
  case ODD:
    otherparity = EVEN ;
    break ;
  case EVENANDODD:
    otherparity = EVENANDODD ;
    break ;
  default:
    node0_printf("ERROR: wrong parity in eigen_stuff::Matrix_Vec_mult\n") ;
    terminate(1) ;
  }

  dslash_field(src , temp, otherparity, fn) ; 
  dslash_field(temp, res , parity     , fn) ;
  FORSOMEPARITY(i,s,parity){ 
    scalar_mult_su3_vector( &(res[i]), -1.0, &(res[i])) ;
  } 
}
/*****************************************************************************/
/* Deallocates the tags and the temporaries the Matrix_Vec_mult needs */
void cleanup_Matrix(){
  cleanup_dslash_temps() ;
  if(temp != NULL ){
    free(temp) ;
    temp = NULL ;
  }
}
#else
/* flag indicating wether to start dslash. */
static int dslash_start = 1 ; /* 1 means start dslash */
/* Message tags to be used for the Matrix Vector multiplication */
static msg_tag *tags1[16],*tags2[16];

/*****************************************************************************/
/* The Matrix_Vec_mult and cleanup_Matrix() using dslash_special */
void Matrix_Vec_mult(su3_vector *src, su3_vector *res, int parity,
		     imp_ferm_links_t *fn ){
  
  register site *s;
  register  int i;
  int otherparity = EVENANDODD;
  /* store last source so that we know when to reinitialize the message tags */
  static su3_vector *last_src=NULL ;

  if(dslash_start){
    temp = (su3_vector *)malloc(sites_on_node*sizeof(su3_vector));
  }

  /*reinitialize the tags if we have a new source */
  if(last_src != src){
    if(!dslash_start) cleanup_gathers(tags1,tags2); 
    dslash_start = 1 ;
    last_src = src ;
  }
  switch(parity){
  case EVEN:
    otherparity = ODD ;
    break ;
  case ODD:
    otherparity = EVEN ;
    break ;
  case EVENANDODD:
    otherparity = EVENANDODD ;
    break ;
  default:
    node0_printf("ERROR: wrong parity in eigen_stuff::Matrix_Vec_mult\n") ;
    terminate(1) ;
  }
  
  dslash_fn_field_special(src , temp, otherparity, tags1, dslash_start, fn) ;
  dslash_fn_field_special(temp, res , parity     , tags2, dslash_start, fn) ;
  
  FORSOMEPARITY(i,s,parity){ 
    scalar_mult_su3_vector( &(res[i]), -1.0, &(res[i])) ;
  } 
  dslash_start = 0 ;
}
/* Deallocates the tags and the temporaries the Matrix_Vec_mult needs */

/************************************************************************/
void cleanup_Matrix(){
  if(!dslash_start) {
    cleanup_gathers(tags1,tags2); 
    cleanup_dslash_temps() ;
    free(temp) ;
  }
  dslash_start = 1 ;
#ifdef DEBUG
  node0_printf("cleanup_Matrix(): done!\n") ; fflush(stdout) ;
#endif
}
#endif /* USE_DSLASH_SPECIAL */

/*****************************************************************************/
/* measures the chiraliry of a normalized fermion state */
void measure_chirality(su3_vector *src, double *chirality, int parity){
  register int i;
  register site *s;
  register double cc ;
  su3_vector *tempvec, *ttt;
  int r0[4] = {0,0,0,0};
  complex tmp ;
  char gam5Xgam1[] = "G1-G5";  /* Note, spin_taste_op inteprets this as G5-G1 */

  ttt = create_v_field();
  tempvec = create_v_field();
  copy_v_field(tempvec, src);
  spin_taste_op(spin_taste_index(gam5Xgam1), r0, ttt, tempvec);

  cc = 0.0 ; 
  FORSOMEPARITY(i,s,parity){
    tmp = su3_dot( tempvec+i, ttt+i );
    cc +=  tmp.real ; /* chirality is real since Gamma_5 is hermitian */
  }
  *chirality = cc ;
  g_doublesum(chirality);

  destroy_v_field(tempvec);
  destroy_v_field(ttt);
}


/*****************************************************************************/
/* prints the density and chiral density of a normalized fermion state */
void print_densities(su3_vector *src, char *tag, int y,int z,int t, 
		     int parity){

  register int i;
  register site *s;
  su3_vector *tempvec, *ttt;
  int r0[4] = {0,0,0,0};
  complex tmp1,tmp2 ;

  ttt = create_v_field();
  tempvec = create_v_field();
  copy_v_field(tempvec, src);
  spin_taste_op(spin_taste_index("G5-G1"), r0, ttt, tempvec);

  FORSOMEPARITY(i,s,parity){ 
    if((s->y==y)&&(s->z==z)&&(s->t==t)){
      tmp1 = su3_dot( tempvec+i, ttt+i ) ;
      tmp2 = su3_dot( tempvec+i, tempvec+i ) ;
      node0_printf("%s: %i %e %e %e\n",tag,
		   s->x,tmp2.real,tmp1.real,tmp1.imag);
    }
  }
}

/*****************************************************************************/
/* Check the residues and norms of the eigenvectors                          */
/* Hiroshi Ono */

void check_eigres(double *resid, su3_vector *eigVec[], double *eigVal,
		  int Nvecs, int parity, imp_ferm_links_t *fn){

  su3_vector *ttt;
  int i,j;
  int otherparity = (parity == EVEN) ? ODD : EVEN;

  /* compute residual and norm of eigenvectors */
  double *norm = (double *)malloc(Nvecs*sizeof(double));
  ttt = create_v_field();
  for(i = 0; i < Nvecs; i++){
    resid[i] = (double)0.0;
    norm[i]  = (double)0.0;
    dslash_fn_field(eigVec[i], ttt, otherparity, fn);
    dslash_fn_field(ttt, ttt, parity, fn);
    FOREVENFIELDSITES(j){
      scalar_mult_sum_su3_vector(ttt+j, eigVec[i]+j, eigVal[i]);
      resid[i] += magsq_su3vec(ttt+j);
      norm[i] += magsq_su3vec(eigVec[i]+j);
    }
  }
  destroy_v_field(ttt);
  g_vecdoublesum(resid, Nvecs);
  g_vecdoublesum(norm, Nvecs);

  node0_printf("Checking eigensolutions\n");
  for(i = 0; i < Nvecs; i++){
    resid[i] = sqrt(resid[i]/norm[i]);
    norm[i] = sqrt(norm[i]);
    node0_printf("eigVal[%d] = %e ( resid = %e , ||eigVec[%d]|| = %e )\n",
		 i, eigVal[i], resid[i], i, norm[i]);
  }
  node0_printf("End of eigensolutions\n");
  free(norm);
}
