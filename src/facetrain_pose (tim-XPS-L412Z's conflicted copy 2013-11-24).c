/*
 ******************************************************************
 * HISTORY
 * 15-Oct-94  Jeff Shufelt (js), Carnegie Mellon University
 *      Prepared for 15-681, Fall 1994.
 *
 * Tue Oct  7 08:12:06 EDT 1997, bthom, added a few comments,
 *       tagged w/bthom
 *
 ******************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pgmimage.h>
#include <backprop.h>

extern char *strcpy();
extern void exit();

main(argc, argv)
int argc;
char *argv[];
{
  char netLeftname[256], netRightname[256], netUpname[256], netStraightname[256], test1name[256];
  IMAGELIST *test1list;
  int ind, list_errors;
  
  list_errors = 0;
  netUpname[0] = netLeftname[0] =netRightname[0] =netStraightname[0] = test1name[0] = '\0';

  if (argc < 2) {
    printusage(argv[0]);
    exit (-1);
  }

  /*** Create imagelists ***/
  test1list = imgl_alloc();

  /*** Scan command line ***/
  for (ind = 1; ind < argc; ind++) {

    /*** Parse switches ***/
    if (argv[ind][0] == '-') {
      switch (argv[ind][1]) {  
        case 's': strcpy(netStraightname, argv[++ind]);
                  break;
      case 'l': strcpy(netLeftname, argv[++ind]);
        break;
		  case 'r': strcpy(netRightname, argv[++ind]);
		    break;
      case 'u': strcpy(netUpname, argv[++ind]);
        break;
        case '1': strcpy(test1name, argv[++ind]);
                  break;
        case 'T': list_errors = 1;
                  break;
        default : printf("Unknown switch '%c'\n", argv[ind][1]);
                  break;
      }
    }
  }

  /*** If any train, test1, or test2 sets have been specified, then
       load them in. ***/
  if (test1name[0] != '\0') 
    imgl_load_images_from_textfile(test1list, test1name);

  /*** If we haven't specified a network save file, we should... ***/
  if (netUpname[0] == '\0' || netLeftname[0] == '\0' || netRightname[0] == '\0' || netStraightname[0] == '\0') {
    printf("%s: Must specify an output file, i.e., -n <network file>\n",
     argv[0]);
    exit (-1);
  }

  /*** Initialize the neural net package ***/
  bpnn_initialize(101492);

  /*** Show number of images in test1 ***/
  printf("%d images in test1 set\n", test1list->n);
 
  
  	/*** store results in an array ***/
	double* confidencesUp;
	double* confidencesLeft;
	double* confidencesRight;
	double* confidencesStraight;
	

  /*** If we've got at least one image to train on, go train the net ***/
  backprop_face(test1list, netUpname, list_errors, &confidencesUp);
	backprop_face(test1list, netRightname, list_errors, &confidencesRight);
  backprop_face(test1list, netLeftname, list_errors, &confidencesLeft);
  backprop_face(test1list, netStraightname, list_errors, &confidencesStraight);
  
  int i;
  printf("Left: \n");
	for(i=0;i<5;i++) {
		printf("confidence %d: %f \n",i,confidencesLeft[i]);
	}
	
	  printf("\nRight: \n");
	for(i=0;i<5;i++) {
		printf("confidence %d: %f \n",i,confidencesRight[i]);
	}
	
	  printf("\nUp: \n");
	for(i=0;i<5;i++) {
		printf("confidence %d: %f \n",i,confidencesUp[i]);
	}
	
	  printf("\nStraight: \n");
	for(i=0;i<5;i++) {
		printf("confidence %d: %f \n",i,confidencesStraight[i]);
	}

  exit(0);
}


backprop_face(test1list, netname,list_errors,confidences)
IMAGELIST *test1list;
int list_errors;
double **confidences;
char *netname;
{
  IMAGE *iimg;
  BPNN *net;
  int i, imgsize;
  double out_err, hid_err, sumerr;

  /*** Read network in if it exists, otherwise make one from scratch ***/
  if ((net = bpnn_read(netname)) == NULL) {
      printf("Need net to run, use -1\n");
      return;
  }

  /*** Print out performance ***/
  printf("\nFailed to classify the following images from the test set 1:\n");
  performance_on_imagelist(net, test1list, 1,confidences);
}


/*** Computes the performance of a net on the images in the imagelist. ***/
/*** Prints out the percentage correct on the image set, and the
     average error between the target and the output units for the set. ***/
performance_on_imagelist(net, il, list_errors,confidences)
BPNN *net;
IMAGELIST *il;
double **confidences;
int list_errors;
{
  *confidences = malloc(il->n*sizeof(double));
  double err, val;
  int i, n, j, correct;

  err = 0.0;
  correct = 0;
  n = il->n;
  if (n > 0) {
    for (i = 0; i < n; i++) {

      /*** Load the image into the input layer. **/
      load_input_with_image(il->list[i], net);

      /*** Run the net on this input. **/
      bpnn_feedforward(net);

      /*** Set up the target vector for this image. **/
      load_target(il->list[i], net);

      /*** See if it got it right. ***/
      if (evaluate_performance(net, &val, 0)) {
        correct++;
      } else if (list_errors) {
	printf("%s - outputs ", NAME(il->list[i]));
	for (j = 1; j <= net->output_n; j++) {
	  printf("%.3f ", net->output_units[j]);
	}
	putchar('\n');
      }
      err += val;
      /*** store the confidence in this decision into an array for later analysis ***/
      (*confidences)[i] = net->output_units[1];
    }

    err = err / (double) n;

    if (!list_errors)
      /* bthom==================================
	 this line prints part of the ouput line
	 discussed in section 3.1.2 of homework
          */
      printf("%g %g ", ((double) correct / (double) n) * 100.0, err);
  } else {
    if (!list_errors)
      printf("0.0 0.0 ");
  }
}

evaluate_performance(net, err)
BPNN *net;
double *err;
{
  double delta;

  delta = net->target[1] - net->output_units[1];

  *err = (0.5 * delta * delta);

  /*** If the target unit is on... ***/
  if (net->target[1] > 0.5) {

    /*** If the output unit is on, then we correctly recognized me! ***/
    if (net->output_units[1] > 0.5) {
      return (1);

    /*** otherwise, we didn't think it was me... ***/
    } else {
      return (0);
    }

  /*** Else, the target unit is off... ***/
  } else {

    /*** If the output unit is on, then we mistakenly thought it was me ***/
    if (net->output_units[1] > 0.5) {
      return (0);

    /*** else, we correctly realized that it wasn't me ***/
    } else {
      return (1);
    }
  }

}



printusage(prog)
char *prog;
{
  printf("USAGE: %s\n", prog);
  printf("       -n <network 1 file>\n");
  printf("       -o <network 2 file>\n");
  printf("       -p <network 3 file>\n");
  printf("       -q <network 4 file>\n");
  printf("       [-s <random number generator seed>]\n");
  printf("       [-1 <testing set 1 list>]\n");
  printf("       [-T]\n");
}
