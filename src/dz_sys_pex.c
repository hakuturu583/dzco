/* DZco - digital control library
 * Copyright (C) 2000 Tomomichi Sugihara (Zhidao)
 *
 * dz_sys_pex - polynomial rational transfer function
 */

#include <dzco/dz_sys.h>
#include <dzco/dz_lin.h>

/* ********************************************************** */
/* polynomial rational transfer function
 * ********************************************************** */

typedef struct{
  int n;
  double *z; /* state variable */
  double *a; /* transient coefficient */
  double *c; /* output coefficient */
  double d;  /* output gain */
  dzPex *pex; /* original polynomial rational (only for memory) */
} dzSysPexPrm;

static dzSysPexPrm *_dzSysPexPrmAlloc(int n);
static void _dzSysPexPrmFree(dzSysPexPrm *prm);

dzSysPexPrm *_dzSysPexPrmAlloc(int n)
{
  dzSysPexPrm *prm;

  if( !( prm = zAlloc( dzSysPexPrm, 1 ) ) ){
    ZALLOCERROR();
    return false;
  }
  prm->z = zAlloc( double, n );
  prm->a = zAlloc( double, n );
  prm->c = zAlloc( double, n );
  if( !prm->z || !prm->a || !prm->c ){
    ZALLOCERROR();
    _dzSysPexPrmFree( prm );
    return NULL;
  }
  prm->n = n;
  return prm;
}

void _dzSysPexPrmFree(dzSysPexPrm *prm)
{
  zFree( prm->z );
  zFree( prm->a );
  zFree( prm->c );
  dzPexDestroy( prm->pex );
  zFree( prm );
}

void dzSysDestroyPex(dzSys *sys)
{
  zArrayFree( dzSysInput(sys) );
  zVecFree( dzSysOutput(sys) );
  _dzSysPexPrmFree( sys->_prm );
  zNameDestroy( sys );
  dzSysInit( sys );
}

void dzSysRefreshPex(dzSys *sys)
{
  memset( ((dzSysPexPrm*)sys->_prm)->z, 0, sizeof(double)*((dzSysPexPrm*)sys->_prm)->n );
}

zVec dzSysUpdatePex(dzSys *sys, double dt)
{
  register int i;
  dzSysPexPrm *prm;
  double v;

  prm = sys->_prm;
  dzSysOutputVal(sys,0) =
    zRawVecInnerProd( prm->c, prm->z, prm->n ) + prm->d*dzSysInputVal(sys,0);
  v = zRawVecInnerProd( prm->a, prm->z, prm->n ) + dzSysInputVal(sys,0);
  for( i=1; i<prm->n; i++ )
    prm->z[i-1] += prm->z[i] * dt;
  prm->z[prm->n-1] += v * dt;
  return dzSysOutput(sys);
}

dzSys *dzSysFReadPex(FILE *fp, dzSys *sys)
{
  dzPex *pex;

  if( !( pex = zAlloc( dzPex, 1 ) ) ){
    ZALLOCERROR();
    return NULL;
  }
  if( !dzPexFRead( fp, pex ) ) return NULL;
  if( !dzSysCreatePex( sys, pex ) ) sys = NULL;
  return sys;
}

void dzSysFWritePex(FILE *fp, dzSys *sys)
{
  dzPexFWrite( fp, ((dzSysPexPrm*)sys->_prm)->pex );
}

dzSysMethod dz_sys_pex_met = {
  type: "pex",
  destroy: dzSysDestroyPex,
  refresh: dzSysRefreshPex,
  update: dzSysUpdatePex,
  fread: dzSysFReadPex,
  fwrite: dzSysFWritePex,
};

/* dzSysCreatePex
 * - create a transfer function from a polynomial rational expression
 *   as an infinite impulse response system.
 */
bool dzSysCreatePex(dzSys *sys, dzPex *pex)
{
  dzLin lin;
  dzSysPexPrm *prm;

  if( !dzPex2LinCtrlCanon( pex, &lin ) ){
    ZRUNERROR( "cannot create Pex" );
    return false;
  }
  dzSysInit( sys );
  dzSysAllocInput( sys, 1 );
  if( dzSysInputNum(sys) == 0 || !dzSysAllocOutput( sys, 1 ) ||
      !( prm = _dzSysPexPrmAlloc( dzLinDim(&lin) ) ) ){
    ZRUNERROR( "cannot create a system" );
    goto TERMINATE;
  }
  zRawVecCopy( zMatRowBuf(lin.a,prm->n-1), prm->a, prm->n );
  zRawVecCopy( zVecBuf(lin.c), prm->c, prm->n );
  prm->d = lin.d;
  prm->pex = pex;

  sys->_prm = prm;
  sys->_met = &dz_sys_pex_met;
  dzSysRefresh( sys );
 TERMINATE:
  dzLinDestroy( &lin );
  return true;
}
