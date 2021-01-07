#include <stdio.h>
#include <stdlib.h>
#include "patoh.h"

int main(int argc, char *argv[])
{
  PaToH_Parameters args;
  int _c, _n, _nconst, *cwghts, *nwghts, *xpins, *pins, *partvec, cut, *partweights;

  if (argc<3) {
    fprintf(stderr, "usage: %s <filename> <#parts>\n", argv[0]);
    exit(1);
  }

  PaToH_Read_Hypergraph(argv[1], &_c, &_n, &_nconst, &cwghts, &nwghts, &xpins, &pins);
  printf("Hypergraph %8s -- #Cells=%6d  #Nets=%6d  #Pins=%8d #Const=%2d\n", argv[1], _c, _n, xpins[_n], _nconst);

  PaToH_Initialize_Parameters(&args, PATOH_CONPART, PATOH_SUGPARAM_DEFAULT);

  args.seed = 1;
  args._k = atoi(argv[2]);
  partvec = (int *) malloc(_c*sizeof(int));
  partweights = (int *) malloc(args._k*_nconst*sizeof(int));
  PaToH_Alloc(&args, _c, _n, _nconst, cwghts, nwghts, xpins, pins);

  PaToH_Part(&args, _c, _n, _nconst, 0, cwghts, nwghts, xpins, pins, NULL, partvec, partweights, &cut);
  printf("seed %d -- %d-way cutsize is: %d\n", args.seed, args._k, cut);

  printf("partvec: ");
  for(int i = 0 ; i<_c ; i++) printf("%d ", partvec[i]);
  printf("\n");

  printf("partweights: ");
  for(int i = 0 ; i<args._k ; i++) printf("%d ", partweights[i]);
  printf("\n");


  free(cwghts);      free(nwghts);
  free(xpins);       free(pins);
  free(partweights); free(partvec);

  PaToH_Free();
  return 0;
}
