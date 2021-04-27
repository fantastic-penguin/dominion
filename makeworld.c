/* makeworld.c   A world generation program by Stephen H. Underwood */
/*
 * Copyright (C) 1990 Free Software Foundation, Inc.
 * Written by the dominion project.
 *
 * This file is part of dominion.
 *
 * dominion is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "dominion.h"
#include "misc.h"
#include <stdio.h>
#include <math.h>
#include <signal.h>
#ifndef ridge			/* ridges don't have limits.h */
# include <limits.h>
#else /* ridge */
# define	INT_MAX		2147483647 /* max decimal value of an "int" */
#endif /* ridge */
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#define HASH_SIZE 500   /* Size of hash table for sea_level determination */
#define STATFILE "world.stats"

extern Sworld world;   
extern Suser user;
extern struct race_list *races;	
extern char libdir[];
extern int (*wrapx)(), (*wrapy)(), compressed_world;
extern struct s_altitude_map altitude_map[];
extern struct item_map climates[];
double **d_map;

/* Base terrains for each given climate type */
int cli_terr[] = { 1, 4, 5, 3, 6, 4, 5, 3, 5, 3, 0 };

void cleanup()
{
}

void critical()
{
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
}
void noncritical()
{
  signal(SIGINT, SIG_DFL);
  signal(SIGQUIT, SIG_DFL);
}

double Gauss()
/* 
   This function returns a gaussian random variable with mean 0
   and a variance of 1.
*/
{
  static int first_time_flag = 0, Nrand = 4;
  static double GaussAdd, GaussFac;
  double sum = 0;
  int i;
  unsigned seed;

  if (first_time_flag == 0)
  {
    /* This initializes things for the gaussing stuff.
       this is done only once. */
    GaussAdd = sqrt((double)(3 * Nrand));
    GaussFac = 2.0 * GaussAdd/ ( (double)Nrand * (double)INT_MAX );
    seed = (unsigned) time(NULL);
    SRND(seed);
    first_time_flag = 1;
  }
  for ( i = 0 ; i < Nrand ; i++) sum += RND();
  return (GaussFac * sum - GaussAdd);

}

int gen_arr(X,H,r,xmax,ymax,max_height, min_height)
double *X[],H,r,*max_height,*min_height;
int xmax,ymax;
/*
   This routine generates a fractal arrray with fratal dimension of 3-H
   and a lacunarity of r.  It is passed the number of sampling points in
   the final array xmax * ymax, and a place to store the value of said
   points. Modified version of a similar code in The Science of Fractal
   Images by Peitgen and Saupe
*/
{
  double delta, xt, yt, xT, yT, h, sigma = 0.5, t, T;
  int  xmT, ymT, xmt, ymt, i, j, indx , xN, yN, x, y;

  xN = xmax + 1;
  yN = ymax + 1;

  if ( xmax < ymax )
  {
    xmT = 2;
    ymT = (int)((double)ymax / ((double)xmax/2.0));
  } else
  {
    ymT = 2; 
    xmT = (int)((double)xmax / ((double)ymax/2.0));
  }
  for (x = 0 ; x < xmT ; x++)
  {
    for (y = 0 ; y < ymT ; y++)
    {
      X[x][y] = 0.0;
    }
  }
  xT = 1.0  / ((double)xmT - 1.0);
  yT = 1.0  / ((double)ymT - 1.0);

  while ((xmT < xN) || (ymT < yN))
  {
    xmt = (int)((double)xmT/r);
    if (xmt == xmT) xmt = xmT + 1;
    if (xmt > xN) xmt = xN;
    xt = 1.0/((double)xmt-1.0);
    
    ymt = (int)((double)ymT/r);
    if (ymt == ymT) ymt = ymT + 1;
    if (ymt > yN) ymt = yN;
    yt = 1.0/((double)ymt-1.0);

    if ( xt > yt)
    {
      t = xt;
      T = xT;
    } else
    {
      t = yt;
      T = yT;
    }
    
    interpolate(X, xmT, ymT, xmt, ymt); 
    delta = pow(t/T, 2.0 - 2.0 * H);
    delta = sqrt(1.0 - delta);
    delta *= sqrt(0.5);
    delta *= pow(t,H);
    delta *= sigma;
    for ( i = 0 ; i < (xmt - 1); i++)
    {
      for ( j = 0 ; j < (ymt - 1); j++)
      {
        X[i][j] += delta * Gauss();
/*
   The following mess is for machinees with NaN implemented rather
   than dumping core on division by zero like it should.  If it
   doesn't apply to your machine, you are lucky and should be grateful.
*/
/*        if (X[i][j] != X[i][j]) /* Yes, I really mean this */
/*        { 
          printf("Error: value %lf at %d %d\n",X[i][j],i,j);
        }
*/
      }
    }
    for (i = 0 ; i <= (xmt - 1) ; i++)
    {
      X[i][ymt-1] = X[i][0];
    }
    for (j = 0 ; j <= (ymt - 1) ; j++)
    {
      X[xmt-1][j] = X[0][j];
    }
    xmT = xmt;
    ymT = ymt;
    xT = 1.0  / ((double)xmT - 1.0);
    yT = 1.0  / ((double)ymT - 1.0);
  }
/* We've computed the fractal, now find the min and max */
  *max_height = -5000;
  *min_height = 5000;
  for (x = 0 ; x < xmax ; x++)
  {
    for (y = 0 ; y < ymax ; y++)
    {
      if (*max_height < X[x][y]) *max_height = X[x][y];
      if (*min_height > X[x][y]) *min_height = X[x][y];
    }
  }
  return delta;
}

void interpolate(X, oldx, oldy, newx, newy)
/*
   Similar to the Interpolate routine mentionedin The Science of
   fractal images this routine interpolates the values of the verticies
   of a larger rectangular grid that covers the same region as the
   smaller rectangular grid stored in X.
   At the beginning X is oldx by oldy and at the end it's newx by newy
*/
double *X[];
int oldx, oldy, newx, newy;
{
  int i,j,x,y,k;
  double *Y, high_per, low_per, right_per, left_per;

/* Get a temporary array to work with. */
   if ((Y = (double *)malloc((oldy+1)* sizeof(double))) == NULL) { mem_error();}

/*
   We work backwards through the array so we don't overwrite rows that
   we still need to work with.  We could make the code simpler by using
   a second array, but that would cost both time (to copy the values back)
   and memory to stor the second array.  Since this routine is called
   frequently, we optimize
*/    

  i = oldx - 1;
  for (x = newx - 1; x > 0 ; x--)
  {
    if ( i * newx >= x * oldx)
    {
/*
   A point always lies between two columns.  We store the right hand
   column in the temporary array Y
*/
      for (k = 0 ; k < oldy; k ++) { Y[k] = X[i][k]; }
      i--;
    }
/* Determine left and right portions for this colmun in the new matrix */ 
    right_per = (double) ( x * oldx - i * newx)/(double)newx;
    left_per = 1.0 - right_per;
/* Loop backwards over y just for consistancy */
    j = oldy - 1;
    for ( y = newy - 1 ; y >= 0 ; y--)
    {
/* Jump down a level, special casing the botom(top) row. */
      if ( j * newy >= y * oldy) { 
       if (--j < 0) { j = 0;}
      }
/* Determine high and low portions for this row in the new matrix */ 
      high_per = (double)(y * oldy - j * newy) / (double)newy;
      low_per = 1.0 - high_per;
/* Now sum up the portions of the 4 corners surrounding this point */
      X[x][y] = ( low_per * left_per * X[i][j] + low_per * right_per * Y[j]
         + high_per * left_per * X[i][j+1] + high_per * right_per * Y[j+1]);
/*
   The following mess is for machinees with NaN implemented rather
   than dumping core on division by zero like it should.  If it
   doesn't apply to your machine, you are lucky and should be grateful.
*/
/*        if (X[x][y] != X[x][y]) /* Yes, I really mean this */
/*        { 
          printf("Error: value %lf at %d %d\n",X[x][y],x,y);
        }
*/
    }
  }
/*
   We special case the last column since we are garunteed an overwrite at 
   that point
*/

  j = oldy;
/* If the 0th column isn't in Y, we put it there now */
  if (i != 0)
  {
    i = 0;
    for (k = 0 ; k < oldy; k ++) { Y[k] = X[i][k]; }
  }
/*
   Compute the 0th columns values.  Note that since it falls on the
   same column as 0th column of the previous matrix, we need only
   concern ourselves with 2-d interpolation
 */
  for ( y = newy - 1 ; y > 0 ; y--)
  {
    if ( j * newy >= y * oldy) { j--; }
    high_per = (double)(y * oldy - j * newy) / (double)newy;
    low_per = 1.0 - high_per;
    X[x][y] = ( low_per * Y[j] + high_per * Y[j+1])/2.0;
  }
/* Clean up after ourselves */
  if ( Y != NULL) free(Y);
}


int init_sector(map,i,j)
/* This function sets a sector to it's base values (mostly zeros) */
Ssector **map;
int i,j;
{
  map[i][j].loc.x = i;
  map[i][j].loc.y = j;
  map[i][j].terrain = 0;
  map[i][j].altitude = 0;
  map[i][j].climate = 0;
  map[i][j].designation = 0;
  map[i][j].soil = 0;
  map[i][j].metal = 0;
  map[i][j].jewels = 0;
  map[i][j].defense = 0;
  map[i][j].roads = 0;
  map[i][j].owner = 0;
  map[i][j].n_people = 0;
  map[i][j].flags = 0;
  map[i][j].name[0] = '\0';
  map[i][j].alist = (struct armyid *)NULL;
};

int init_gamemaster(np)
/* 
   This function sets up the gamemaster nation (the only nation the game
   starts with).
*/
Snation *np;
{
  char temppass[PASSLEN], temp2pass[PASSLEN];

  np->id = 0;
  strcpy(np->name,"Gamemaster");
  sprintf(np->leader,"%s","Exalted One");
/* We need to get the password from the user */
  get_crypt_pass("\nGive World/Gamemaster password: ", temppass, NULL, NULL);
  get_crypt_pass("\nType it once more: ", temp2pass, NULL, NULL);
  while  (strcmp(temppass, temp2pass) != 0){
    fprintf(stderr,"\nThose two didn't match.  Please try again.\n");
    get_crypt_pass("\nGive World/Gamemaster password: ", temppass, NULL, NULL);
    get_crypt_pass("\nType it once more: ", temp2pass, NULL, NULL);
  }
  strcpy(np->passwd, temppass);
  np->capital.x = 0;
  np->capital.y = 0;
  np->race = races->race;  /* Set's it to the first race, Master */
  np->mark = '-';          /* Useful for display nation displays */
/* 
  Everything starts at 0, since if the gamemaster wants something,
  he/she can use the edit nation feature to add it.
*/
  np->taxes = 0;
  np->taxtype = 0;
  np->charity = 0;
  np->money = 0;
  np->jewels = 0;
  np->metal = 0;
  np->food = 0;
  np->n_sects = 0;
  np->tech_r_d = 0;
  np->tech_r_d_metal = 0;
  np->mag_r_d = 0;
  np->mag_r_d_jewels = 0;
  np->spy_r_d = 0;
  np->npc_flag = 0;
  np->npc_agg = 0;
  np->npc_exp = 0;
  np->npc_iso = 0;
  strcpy(np->mag_order,"Master");
  np->tech_skill = 0;
  np->mag_skill = 0;
  np->farm_skill = 0;
  np->mine_skill = 0;
  np->spell_pts = 1;
  np->attack = 0;
  np->defense = 0;
  np->spy = 0;
  np->secrecy = 0;
  np->n_armies = 0;
  np->armies = (Sarmy *) NULL;
  np->ptlist = (struct pt_list *) NULL;

  return 0;
}

int init_d_map(xmax,ymax)
/*
   Allocate the memory for the temporary array the size of the world
   (with one extra for wrap around) for manipulation before scaling 
*/
int xmax,ymax;
{
  int i;
  
  if ((d_map = (double **) malloc((xmax + 1) * (sizeof(double *)))) == NULL)
  {
    mem_error();
  }
  for (i=0; i <=xmax ; i++)
  {
    if ((d_map[i] = (double *) malloc((ymax + 1) * (sizeof(double)))) == NULL)
    {
      mem_error();
    }
  }
}

double find_level(wp,d_map,perc, max_height, min_height)
/*
   This function finds the level beneath which are roughly "perc" 
   percentage of the sectors available.  Good for setting sea
   level if you want 30% sea for example.
*/
Sworld *wp;
double *d_map[],perc,max_height,min_height;
{
  static int hashes[HASH_SIZE + 1];
  int num_sect,i,k,x,y, ltemp;
  double level, scale_factor;


  if (perc == 1.0) /* If it's all of them, but the level above everything */
  {
    level = max_height;
    return level;
  }
  if (perc == 0.0) /* If it's none of them, put the level below everything */
  {
    level = min_height;
    return level;
  }
/* Empty the hash table */
  for (i=0;i<=HASH_SIZE;i++) hashes[i] = 0;
/* Loop over the array, filing the hash table to the number of sectors
   in the region each hash slot is associated with.
*/ 
  scale_factor = (double)HASH_SIZE/
            ((max_height - min_height)*(1.0 + 1.1/HASH_SIZE));

  for (x = 0 ; x < wp->xmax ; x++)
  {
    for (y = 0 ; y < wp->ymax ; y++)
    {
      ltemp = (d_map[x][y] - min_height) * scale_factor;
/*      if ((ltemp < 0 ) || (ltemp >= HASH_SIZE))
      {
        printf("Error: ltemp %d max %lf min %lf val %lf\n",ltemp,
                   max_height, min_height, d_map[x][y]);
      } else 
*/
      {
        hashes[ltemp]++;
      }
    }
  }
/* Determine the number of sectors below the level */
  num_sect = (int)((double)(wp->xmax * wp->ymax) * perc);
  k = 0; i = -1;
/* Find how far down the hash table we have to go to get that many sectors */
  while ( ( k < num_sect) && ( i <= HASH_SIZE))
  {  
    i++;
    k += hashes[i]; 
  }
  if (i >= HASH_SIZE) /* If it's all of them, then use all of them */
  {
    level = max_height;
  }
  /* if it's none of them, use none of them. */
  else if ((i == 0) || (num_sect == 0))
  {
    level = min_height;
  }
  else 
/* 
   Estimate the level as half way between a bit too few
   (at the i-1 level) and a few too many (at the ith level)
*/
  {
   level = ((max_height - min_height) * (((double)i - 0.5) / 
                        (double)HASH_SIZE) ) + min_height;
  }

  return level;
}

void power_scale(wp,d_map,level, k, minimum, maximum)
/* 
  This function takes the entire map and rescales it by taking
  each item to the kth power, after rescaling the value to be
  between 1 and 2.  This means that all the points get lower,
  but specifically that high points loose more height than
  the lower points.  This brings most of the mountain peaks
  down to within rougly the same range.
*/
Sworld *wp;
double *d_map[],level,k,*maximum, *minimum;
{
  int x,y;
  double old_min = *minimum , old_max = *maximum, htemp;

  *maximum = -50; *minimum = 50; 
  for (x = 0 ;x < wp->xmax ; x++)
  {
    for (y = 0 ; y < wp->ymax ; y++)
    {/* scale us relative to 0 */
      if ((htemp = (d_map[x][y] -= level)) < 0)
      {
/*
   scale below water between -1 and 0, then between 1 and 2, take power
   and then scale back below 0.  Remember roots of a negative number may
   be meaninless, so it has to be shifted (as we have done) into the positive
*/
        if (old_min != level)
        { 
          htemp /= (old_max - level);
	} else
        {
          htemp = 0;
	}
        htemp = 1 - pow(1 - htemp,k);
      } else
      { 
/*
   scale above water between 0 and 1, and then between 1 and 2 and then
   after the power back between 0 and 1 
*/
        if (old_max != level)
        { 
          htemp /= (old_max - level);
	} else
        {
          htemp = 0;
	}
        htemp = pow(1 + htemp,k) -1;
      }
      if (htemp > *maximum) *maximum = htemp;
      if (htemp < *minimum) *minimum = htemp;
      d_map[x][y] = htemp;
    }
  }
}

void gen_alt(wp,water_per,dim,lac,info_flag,fp)
/* 
   Here we generate the altitudes for the world.  First step is to create
   a fractal array of doubles to scale.  We find where sea level should be,
   and then scale up and down from sea level.
*/
double dim,lac,water_per;
int info_flag;
Sworld *wp;
FILE *fp;
{
  double max_height, min_height,sea_level,dry_factor,wet_factor,old_min;
  double loc_min,loc_max;
  int i,j,x,y,k,sea_perc,htemp,alt_hash[14];
  int loc_x, loc_y;

  printf("Generating Altitudes\n");
  wp->geo.pwater = (int)(water_per * 100.0);
  gen_arr(d_map,dim,lac,wp->xmax,wp->ymax,&max_height,&min_height);
  old_min = min_height;

/* Clear out the hash table, and then fill it again by level of map */
  sea_level = find_level(wp,d_map,water_per, max_height, min_height);

/*
   Rescale by taking everything to the 0.5 power.  flattens out
   the higher peaks
*/
  power_scale(wp,d_map,sea_level, 0.5 ,&min_height, &max_height);
  sea_level = 0.0; /* Do power scales things to sea level for us */

/* Scaling factors are determined for below and above sea_level */
  dry_factor = (int)(13.0 / (max_height - sea_level));
  wet_factor = (int)(7.0 / (sea_level - min_height));
  sea_perc = 0;  /* redetermine the exact sea_percentage */
  for (i=0; i < 13 ; i++) alt_hash[i] = 0; /* set the hash table to zero */
/* Make one last loop over the world */
  for (x = 0 ; x < wp->xmax ; x++)
  {
    for (y = 0 ; y < wp->ymax ; y++)
    {
      d_map[x][y] = d_map[x][y] - sea_level;
      if ((d_map[x][y] >= 0.0) && (water_per < 1.0))/* Dry land */
      {
        htemp = (int) (d_map[x][y] * dry_factor);
        htemp++; /* We want 50% less lowlands */
        if (htemp > 8) htemp--; /* We want 50% more mountains */
        if (htemp > 10) htemp--; /* We want 50% more peaks */
        htemp /= 2;
        if (htemp > 5)
        {/* If somehow the scaling is fucked */
          printf("htemp error %d at %d %d\n",htemp,x,y);
          fprintf(fp,"htemp error %d at %d %d\n",htemp,x,y);
          htemp = 6;
        }
        else htemp++;  /* needs to be between 1 and 6, not 0 and 5 */
        wp->map[x][y].altitude = htemp;
        alt_hash[htemp + 5]++;  /* one more sector of htemp altitude */
      } else if (d_map[x][y] < 0.0) /* Dry land */
      {
        sea_perc++;           /* One more sea sector */
        htemp = (int)(d_map[x][y] * wet_factor);
        htemp--;  /* number need to be -1 to -6 not 0 to -5 */
        if (htemp < OCEAN_PLAINS ) htemp++;  /* Makes more ocean_plains */
        if (htemp < TRENCH ) htemp++;  /* Makes more trench */
        if (htemp < -5)
        { /* If somehow the scaling if fucked. */
          printf("htemp error %d at %d %d\n",htemp,x,y);
          fprintf(fp,"htemp error %d at %d %d\n",htemp,x,y);
          htemp = -5;
        } 
        wp->map[x][y].altitude = htemp; 
        alt_hash[htemp + 5]++;  /* one more sector of htemp altitude */
      }
    }
  }
/* Now determine the sea percentage more exactly, rounding correctly */
  sea_perc = (int)((((double)sea_perc * 100.0) /
                (double)(wp->xmax * wp->ymax)) + 0.49);
  if (info_flag == 3)
  {
    printf("Percentage of water : %d%%\n",sea_perc);
    for (i= 0 ; i <= 11 ; i++) {
      if ( i != 5) {
        printf("%d sectors are %s\n",alt_hash[i],altitude_map[i].name);
      } 
    }
  }
  if (fp != NULL) {
    fprintf(fp,"Altitude Fractal Dimension : %lf\n",3.0 - dim);
    fprintf(fp,"Percentage of water : %d%%\n",sea_perc);
    for (i= 0 ; i <= 11 ; i++) {
     if (i != 5) {
      fprintf(fp,"%d sectors are %s\n",alt_hash[i],altitude_map[i].name);
     }
    }
  }
}

void gen_climate(dim,lac,wp,info_flag,fp)
/* 
   Here we generate the climates for the world.  First step is to create
   a fractal array of doubles to scale.  Then we modify everything by
   multiplying it by a sin function, thereby producing the lowest values
   (0) at the beginning and the end.  Then we add a linear factor
   into the polar regions, and a constant factor to everywhere else
   to raise things up a bit.  Lastly we scale things over 11 climates
   for land, and 9 climates for water (water has no deserts)
   When we determine the climate, a certain base terrain type goes
   with it.  That gets adjusted later.
*/
double dim,lac;
int info_flag;
Sworld *wp;
FILE *fp;
{
  double max_height, min_height,dtemp,old_min,old_max;
  int i,x,y,htemp,artic_region,hashes[12];

  printf("Generating Climates\n");
/* Generate the original array */
  gen_arr(d_map,dim,lac,wp->xmax,wp->ymax,&max_height,&min_height);
  old_min = min_height; max_height = -50; min_height = 50;
/* Do the sin function filter */
  for (x = 0 ; x < wp->xmax ; x++)
  {
    for (y = 0 ; y < wp->ymax ; y++)
    {
      d_map[x][y] = d_map[x][y] - old_min;
      d_map[x][y] = sin((double)y * M_PI/(double)(wp->ymax)) * d_map[x][y];
      if (d_map[x][y] < min_height ) min_height = d_map[x][y];
      if (d_map[x][y] > max_height ) max_height = d_map[x][y];
    }
  }
  old_max = max_height; old_min = min_height;
  max_height = -50; min_height = 50;
/* Scale things linearly up over the artic and polar regions */
  artic_region = wp->ymax / 5;
  for (x = 0 ; x < wp->xmax ; x++)
  {
    for (y = 0 ; y < wp->ymax ; y++)
    {
      dtemp = (old_max - old_min)/9.0;
      if (y < artic_region)
      { 
        d_map[x][y] += dtemp * (double)y / (((double)artic_region)/2.0);
      }
      else if ((wp->ymax - y) < artic_region)
      { 
        d_map[x][y] += dtemp*(double)(wp->ymax-y)/((double)artic_region/2.0);
      }
      else
      {
        d_map[x][y] += 2.0 * dtemp;
      }
      if (d_map[x][y] > max_height) max_height = d_map[x][y];
      if (d_map[x][y] < min_height) min_height = d_map[x][y];
    }
  }

/* Standard scaling */
  for (i=0;i<11;i++) hashes[i] = 0;
  for (x = 0 ; x < wp->xmax ; x++)
  {
    for (y = 0 ; y < wp->ymax ; y++)
    {
      if (wp->map[x][y].altitude < 0)
      {
        htemp = (int)(((d_map[x][y]-min_height)*9.0)/(max_height-min_height));
      } else
      {
        htemp = (int)(((d_map[x][y]-min_height)*12.0)/(max_height-min_height));
        if (htemp > 10) htemp--; /* We want more desert */
      }
      if (htemp >= 11)
      {
       htemp = 10;
      }
      htemp = 11 - (htemp + 1);
      wp->map[x][y].climate = htemp;
      if (wp->map[x][y].altitude < 0)
      {
/* If it's an ocean be terrain type ocean. */
         wp->map[x][y].terrain = -5;
      } else
      {
        wp->map[x][y].terrain = cli_terr[htemp];
      }
      hashes[htemp]++;
    }
  }
  if (info_flag == 3)
  {
    for (i= 0 ; i < 11 ; i++)
    {
      printf("%d sectors have %s climate \n",hashes[i],climates[i].name);
    }
  }
  if (fp != NULL)
  {
    fprintf(fp,"\nClimate Fractal Dimension: %lf\n",3.0 - dim);
    for (i = 0 ; i < 11 ; i++)
    {
      fprintf(fp,"%d sectors have %s climate \n",hashes[i],climates[i].name);
    }
  }
}

void adjust_terrain(wp)
/*
   This function adjusts the terrain starting at the base level
   given by the climate, and adjusting for altitude.
*/
Sworld *wp;
{
  int x,y;

  for (x = 0 ; x < wp->xmax; x++)
  {
    for (y = 0 ; y < wp->ymax ; y++)
    {
      if (wp->map[x][y].altitude > HILLS)
      {
        wp->map[x][y].terrain--;
        if (wp->map[x][y].altitude > MOUNTAINS)
        {
          wp->map[x][y].terrain--;
        }
        if (wp->map[x][y].terrain < ICE) wp->map[x][y].terrain = ICE;
        else if (wp->map[x][y].terrain == SWAMP) wp->map[x][y].terrain= BARREN;
      } 
      else if (wp->map[x][y].altitude == LOWLANDS)
      {
        if ((wp->map[x][y].climate > 1 ) && (wp->map[x][y].climate < 9))
        {
          if ((wp->map[x][y].terrain != FOREST) && (RND() % 4 == 0))
          {
            wp->map[x][y].terrain = SWAMP;
          }
        }
      }
    }
  }
}

void gen_metal(wp,metal_per,metal_avg,info_flag,fp)
/*
  This function distributes the metal over the world.  The location of
  the metal deposits is determined fractally, and the value of the
  deposits is determined randomly, with a gaussian distribution.  The
  average metal in a sector is metal_avg and the percentage of sectors
  with any metal at all is metal_per
*/
double metal_per,metal_avg;
int info_flag;
Sworld *wp;
FILE *fp;
{
  double maximum, minimum,dtemp,old_min,old_max,zero_level;
  int i,x,y,htemp,hashes[41],k, tot_sect = 0, tot_met = 0, max_met = 0;

  printf("Generating Metal\n");
  wp->geo.metal_avg = metal_avg;
  minimum = 10000.0; maximum = -13000.0;
  for (x = 0 ; x < wp->xmax ; x++)
  {
    for (y = 0 ; y < wp->ymax ; y++)
    {
      d_map[x][y] = (double)(RND() % 10000) + 1000.0;
      switch(wp->map[x][y].altitude) 
      {
/* Hill, and the like are more probable to have metal */
        case HILLS:
        case MOUNTAIN_PEAK:
        case TRENCH:
        case CONT_SHELF: d_map[x][y] += 1000; break;
/* mountains are even more probable to have metal */
        case SEA_MOUNT:
        case MOUNTAINS: d_map[x][y] += 2000; break;
/* Plains lowlands and shallows are less probable */
        case PLAINS:
        case LOWLANDS:
        case SHALLOWS: d_map[x][y] -= 1000; break;
      }
      if (minimum > d_map[x][y]) minimum = d_map[x][y];
      if (maximum < d_map[x][y]) maximum = d_map[x][y];
    }
  }
/* Find the cutoff level for metal */
  zero_level = find_level(wp,d_map,metal_per, maximum, minimum);

  for (i=0;i<40;i++) hashes[i] = 0;
/* Then fill all the sectors that have metal with some amount of metal */
  for (x = 0 ; x < wp->xmax ; x++)
  {
    for (y = 0 ; y < wp->ymax ; y++)
    {
      if (d_map[x][y] > zero_level)
      {
        dtemp = fabs((Gauss() + Gauss()))/2.0;
        dtemp *= metal_avg;
        dtemp /= 0.651;
        htemp = (int)(dtemp) + 1;
        wp->map[x][y].metal = htemp;
        tot_sect++;
        tot_met += htemp;
        if (max_met < htemp) { max_met = htemp; }
        if (htemp > 20) { hashes[20]++; }
        else { hashes[htemp]++; }
      } else hashes[0]++;
    }
  }
  if (info_flag == 3)
  {
    printf("There are %d sectors with metal\n",tot_sect);
    max_met = min(max_met, 20);
    for (i= 0 ; i < max_met  ; i++)
    {
      printf("There are %d sectors with %d metal\n",hashes[i],i);
    }
    if (max_met < 20)
    {
      printf("There are %d sectors with %d metal\n",hashes[max_met],max_met);
    } else
    {
      printf("There are %d sectors over %d metal\n",hashes[max_met],max_met-1);
    }
  }
  if (fp != NULL)
  { 
    fprintf(fp,"\nMetal Chance %d%% with %lf average\n",
              (int)((1.0 - metal_per) * 100.0),metal_avg);
    fprintf(fp,"Total Metal %d over %d sectors with %d maximum\n",
            tot_met,tot_sect ,max_met);
    max_met = min(max_met, 20);
    for (i= 0 ; i < max_met  ; i++)
    {
      fprintf(fp,"There are %d sectors with %d metal\n",hashes[i],i);
    }
    if (max_met < 20)
    {
      fprintf(fp,"There are %d sectors with %d metal\n",hashes[max_met],
             max_met);
    } else
    {
      fprintf(fp,"There are %d sectors over %d metal\n",hashes[max_met],
               max_met-1);
    }
  }

}

void gen_jewel(wp,jewel_per,jewel_avg,info_flag,fp)
/* 
   This adds jewels to the world the same way metal is added.
   If you understood gen_metal, you understand this.  They are
   basically the same.
*/
double jewel_per, jewel_avg;
int info_flag;
Sworld *wp;
FILE *fp;
{
  double maximum, minimum,dtemp,old_min,old_max,zero_level;
  int i,x,y,htemp,hashes[41],k,tot_sect = 0, tot_jewels = 0, max_jewel = 0; 

  printf("Generating Jewels\n");
  wp->geo.jewel_avg = jewel_avg;
  minimum = 10000.0; maximum = -13000.0;
  for (x = 0 ; x < wp->xmax ; x++)
  {
    for (y = 0 ; y < wp->ymax ; y++)
    {
      d_map[x][y] = (double)(RND() % 10000) + 1000.0;
      switch(wp->map[x][y].altitude) 
      {
        case HILLS:
        case MOUNTAIN_PEAK:
        case TRENCH:
        case CONT_SHELF: d_map[x][y] += 1000; break;
        case SEA_MOUNT:
        case MOUNTAINS: d_map[x][y] += 2000; break;
        case PLAINS:
        case LOWLANDS:
        case SHALLOWS: d_map[x][y] -= 1000; break;
      }
      if (minimum > d_map[x][y]) minimum = d_map[x][y];
      if (maximum < d_map[x][y]) maximum = d_map[x][y];
    }
  }
  zero_level = find_level(wp,d_map,jewel_per, maximum, minimum);
  for (i=0;i<40;i++) hashes[i] = 0;
  for (x = 0 ; x < wp->xmax ; x++)
  {
    for (y = 0 ; y < wp->ymax ; y++)
    {
      if (d_map[x][y] > zero_level)
      {
        dtemp = fabs((Gauss() + Gauss()))/2.0;
        dtemp *= jewel_avg;
        dtemp /= 0.651;
        htemp = (int)(dtemp) + 1;
        wp->map[x][y].jewels = htemp;
        tot_sect++;
        tot_jewels += htemp;
        if (max_jewel < htemp) { max_jewel = htemp; }
        if (htemp > 20) { hashes[20]++; }
        else { hashes[htemp]++; }
      } else hashes[0]++;
    }
  }
  if (info_flag == 3)
  {
    printf("There are %d sectors with jewels\n\n",tot_sect);
    max_jewel = min(max_jewel, 20);
    for (i= 0 ; i < max_jewel  ; i++)
    {
      printf("There are %d sectors with %d jewels\n",hashes[i],i);
    }
    if (max_jewel < 20)
    {
      printf("There are %d sectors with %d jewels\n",hashes[max_jewel],
               max_jewel);
    } else
    {
      printf("There are %d sectors over %d jewels\n",hashes[max_jewel],
             max_jewel-1);
    }
  }
  if (fp != NULL)
  {
    fprintf(fp,"\nJewels Percentage %d%% at %lf average\n",
                  (int)((1.0 - jewel_per) * 100.0),jewel_avg);
    fprintf(fp,"Total %d Jewels over %d sectors with %d max\n",
               tot_jewels, tot_sect, max_jewel);
    max_jewel = min(max_jewel, 20);
    for (i= 0 ; i < max_jewel  ; i++)
    {
      fprintf(fp,"There are %d sectors with %d jewels\n",hashes[i],i);
    }
    if (max_jewel < 20)
    {
      fprintf(fp,"There are %d sectors with %d jewels\n",hashes[max_jewel],
                max_jewel);
    } else
    {
       fprintf(fp,"There are %d sectors over %d jewels\n",hashes[max_jewel],
              max_jewel-1);
    }
  }
}

void gen_soil(wp,dim,lac,average,info_flag,fp)
/* 
   This routine sets the soil value for each sector.  To some degree
   this is done fractally, and to some degree it is determined by
   the terrain type and altitude of the sector in question.  As well
   lots of metal and jewels in the region will limit the ammount of
   food obtainable
*/
double dim,lac,average;
int info_flag;
Sworld *wp;
FILE *fp;
{
  double max_soil, min_soil,dtemp,old_min,old_max, total = 0.0, avg ;
  int i,x,y,htemp,hashes[41], max_s = 0;

  printf("Generating Soil\n");
  wp->geo.soil_avg = average;
  gen_arr(d_map,dim,lac,wp->xmax,wp->ymax,&max_soil,&min_soil);
  old_min = min_soil; max_soil = -50; min_soil = 50;
/* Generate the basic soil fertility by fractal methods */
  for (x = 0 ; x < wp->xmax ; x++)
  {
    for (y = 0 ; y < wp->ymax ; y++)
    {
      d_map[x][y] = d_map[x][y] - old_min;
      d_map[x][y] = (d_map[x][y]/ (old_max- old_min)) * 8.0;
/* 
      d_map[x][y] = sin(((double)(y* M_PI)/(double)(wp->ymax*1.5))+ (M_PI/6.0))
                    * d_map[x][y];
*/
      if (d_map[x][y] < min_soil ) min_soil = d_map[x][y];
      if (d_map[x][y] > max_soil ) max_soil = d_map[x][y];
    }
  }
  for (i=0;i<40;i++) hashes[i] = 0;
  for (x = 0 ; x < wp->xmax ; x++)
  {
    for (y = 0 ; y < wp->ymax ; y++)
    {
/* Modify food production by altitude */
      switch(wp->map[x][y].altitude) 
      {
        case TRENCH:         d_map[x][y] -= 2.0;  break;
        case SEA_MOUNT:      d_map[x][y] -= 1.0;  break;
        case OCEAN_PLAINS:   d_map[x][y] += 0.0;  break;
        case CONT_SHELF:     d_map[x][y] += 1.0;  break;
        case SHALLOWS:       d_map[x][y] += 2.0;  break;
        case LOWLANDS:       d_map[x][y] += 2.0;  break;
        case PLAINS:         d_map[x][y] += 1.0;  break;
        case HILLS:          d_map[x][y] -= 0.5;  break;
        case PLATEAU:        d_map[x][y] -= 0.2;  break;
        case MOUNTAINS:      d_map[x][y] -= 1.0;  break;
        case MOUNTAIN_PEAK:  d_map[x][y] -= 2.0;  break;
      }
/* Modify food production by climate type */
      switch(wp->map[x][y].altitude) 
      {
        case 0:         d_map[x][y] -= 1.5;  break;
        case 1:         d_map[x][y] -= 0.7;  break;
        case 2:         d_map[x][y] += 0.0;  break;
        case 3:         d_map[x][y] += 0.0;  break;
        case 4:         d_map[x][y] += 0.0;  break;
        case 5:         d_map[x][y] += 0.0;  break;
        case 6:         d_map[x][y] += 0.6;  break;
        case 7:         d_map[x][y] += 0.2;  break;
        case 8:         d_map[x][y] += 0.0;  break;
        case 9:         d_map[x][y] -= 0.5;  break;
        case 10:        d_map[x][y] -= 1.1;  break;
      }
   
/* Modify food production by terrain type */
      switch(wp->map[x][y].terrain)
      {
        case -5:  d_map[x][y] += 0.0;  break;
        case -4:  d_map[x][y] += 0.0;  break;
        case -3:  d_map[x][y] += 0.0;  break;
        case -2:  d_map[x][y] += 1.0;  break;
        case -1:  d_map[x][y] += 1.0;  break;
        case 0 :  d_map[x][y] -= 2.0;  break;
        case 1:   d_map[x][y] -= 1.0;  break;
        case 2:   d_map[x][y] += 0.0;  break;
        case 3:   d_map[x][y] += 1.0;  break;
        case 4:   d_map[x][y] += 1.5;  break;
        case 5:   d_map[x][y] += 2.0;  break;
        case 6:   d_map[x][y] += 2.5;  break;
      }
/* Remove food for excess metals and jewels, rounding down */
      d_map[x][y] -= (int)(wp->map[x][y].metal / 8);
      d_map[x][y] -= (int)(wp->map[x][y].jewels / 8);
/* Give the people beneath the see food for fishing */
      if (wp->map[x][y].altitude < 0)
      {
         d_map[x][y] += 1;  /* Fish */
      }
/* Minimum soil is 0 */
      if (d_map[x][y] < 0 )
      {
        d_map[x][y] = 0;
      }
      total += d_map[x][y];
    }
  }
  avg = total / (double)(wp->xmax * wp->ymax);
  for (x = 0 ; x < wp->xmax ; x++)
  {
    for (y = 0 ; y < wp->ymax ; y++)
    {
      htemp = (int)(((d_map[x][y]/avg)*average) + 0.49);
      if (max_s < htemp) { max_s = htemp;}
      if (htemp > 20) { hashes[20]++; }
      else { hashes[htemp]++; }
      wp->map[x][y].soil = htemp;
    }
  }
  if (info_flag == 3)
  {
    max_s = min(max_s, 20);
    for (i= 0 ; i < max_s  ; i++)
    {
      printf("There are %d sectors with %d soil\n",hashes[i],i);
    }
    if (max_s < 20)
    {
      printf("There are %d sectors with %d soil\n",hashes[max_s],max_s);
    } else
    {
      printf("There are %d sectors over %d soil\n",hashes[max_s],max_s-1);
    }
  }
  if (fp != NULL)
  {
    fprintf(fp,"\nSoil Adjustment Factor %lf Maximum is %d\n",average,max_s);
    max_s = min(max_s, 20);
    for (i= 0 ; i < max_s  ; i++)
    {
      fprintf(fp,"There are %d sectors with %d soil\n",hashes[i],i);
    }
    if (max_s < 20)
    {
      fprintf(fp,"There are %d sectors with %d soil\n",hashes[max_s],max_s);
    } else
    {
      fprintf(fp,"There are %d sectors over %d soil\n",hashes[max_s],max_s-1);
    }
  }
}

void explain(help_num)
int help_num;
{
  switch(help_num)
  {
    case 0:
    {
      printf("The fractal dimenstion of a given region determines it's\n");
      printf("relative smoothness. A number close to 2 will be very smooth\n");
      printf("as the surface is trying to get closer to a plane (a two \n");
      printf("dimensional object) and as it gets closer to 3 it will\n");
      printf("become more and more jagged, trying to approximate a space\n");
      printf("filling surface.  The value must be between 2 and 3\n");
      printf("but not equal to either 2 or 3\n");
      break;
    }
    case 1:
    {
      printf("Percentages are integers between 0 and 100 inclusive\n");
      break;
    }
    case 2:
    {
      printf("A default world is one in which the default settings are\n");
      printf("used with respect to world design.   The user is left with\n");
      printf("the options concerning width, height, and water percentage\n\n");
      printf("A customized world allows the user to choose the options at\n");
      printf("each step in the process (such as metal level etc.)\n\n");
      printf("A customized world with confirmation will use the values\n");
      printf("given to create a world, and then will show some statistics\n");
      printf("at each step, asking the user to confirm that that is indeed\n");
      printf("what they had in mind.  This is the most verbose option.\n");
      break;
    }
    case 4:
    {
      printf("Both the height and width of the world must be integers\n");
      printf("which are at least 8 and at most whatever you machine can\n");
      printf("handle.  Be aware that there is a lot of memory usage per\n");
      printf("sector, so you may run out of swap space and have to restart\n");
      printf("if you are overly optimistic.\n");
      break;
    }
    default:
    {
      printf("No help is available for this topic at this time \n");
      break;
    }
  }
}

int get_int(prompt, help_num, value)
char *prompt;
int help_num, *value;
{
  int rtvl;
  char tmp[100];
  
  rtvl = -100;
  while (rtvl == -100)
  {
    printf("%s",prompt);
    fgets(tmp, 99,stdin);
    if (sscanf(tmp,"%d",value) == 1)
    {
      rtvl = 0;
    } else if (tmp[0] == '?')
    {
      explain(help_num);
    }
  }
  return rtvl;
}

int get_double(prompt, help_num, value)
char *prompt;
int help_num;
double *value;
{
  int rtvl;
  char tmp[100];
  
  rtvl = -100;
  while (rtvl == -100)
  {
    printf("%s",prompt);
    fgets(tmp, 99,stdin);
    if (sscanf(tmp,"%lf",value) == 1)
    {
      rtvl = 0;
    } else if (tmp[0] == '?')
    {
      explain(help_num);
    }
  }
  return rtvl;
}

void set_compressed()
{
  char s[100];

  printf("\nDo you wish the world file to be compressed? ");
  fgets(s,99,stdin);
  if ((s[0] == 'y') || (s[0] == 'Y'))
  {
    compressed_world = 1;
  }
  printf("\nYou can change your mind at any time by manually compressing or\n");
  printf("uncompressing the file %s/%s\n\n\n",libdir,WORLD_FILE);
}


void main (argc, argv)
     int argc;
     char *argv[];
{
  extern char *optarg;
  extern int optind;
  int i,j, percent, info_flag = 0, conf_flag;
  int c;
  double fracdim = -1.0,lacun = (2.0/3.0), perc_d, average;
  FILE *fp;

  strcpy(libdir, DEF_LIBDIR);

  while ((c = getopt(argc, argv, "d:--")) != EOF)
  {
    switch (c)
    {
      case 'd':
        strcpy(libdir, optarg);
        break;
    }
  }

  if (chdir(libdir) == -1) {
    fprintf(stderr,"Error: cannot cd to directory %s\n",libdir);
    clean_exit();
    exit();
  }
  SRND(time(0L));		/* initialize random number generator */
  read_races();			/* get races from races file */
  world.turn = 0;
  if ((fp = fopen(STATFILE,"w")) == NULL)
  {
    fprintf(stderr,"Error: Could not write to specified directory\n");
    fprintf(stderr,"Please check permissions and try again\n");
    clean_exit();
    exit(1);
  }

  printf("At any prompt hit ? and return to get more information\n");
  printf("(if there is anything more available)\n\n");

  printf("Do you wish 1) A default settings world 2) A customized world\n");
  printf("         or 3) A customized world with confirmed completions \n");
  
  info_flag = -1;
  while ((info_flag < 0) || (info_flag > 3))
  {
    get_int("Choice (1,2,3) : ",2,&info_flag);
  }

  printf("\nThe world will be shaped like a torus, as that is the only\n");
  printf("shape implemented as of yet.\n");
  world.geo.topology = TORUS;
  init_wrap();

  world.xmax = -1;
  while (world.xmax < 8 )
  {
    get_int("Please enter the width of the world (min 8): ",4,&(world.xmax));
  }
  world.ymax = -1;
  while (world.ymax < 8 )
  {
    get_int("Please enter the height of the world (min 8): ",4,&(world.ymax));
  }
  
  init_d_map(world.xmax,world.ymax);
  if ((world.map = (Ssector **)malloc(sizeof(Ssector *) * world.xmax)) == NULL)
  {
    mem_error();
  }
  for (i=0;i<world.xmax;i++)
  {
    if ((world.map[i] = (Ssector *)malloc(sizeof(Ssector)*world.ymax)) == NULL)
    {
      mem_error();
    }
  }
  for (i=0;i<world.xmax;i++)
  {
    for (j=0;j<world.ymax;j++)
    {
      init_sector(world.map,i,j);
    }
  }

  conf_flag = 0;
  while (conf_flag != 1)
  {
   if (info_flag == 1)
   {
     fracdim = 2.75;
   } else
   {
    if (conf_flag != 2)
    {
     printf("\nThe altitude will be determined using fractal methods. You\n");
     printf("should enter the value for the fractal dimension of the\n");
     printf("world. The number should be between 2 and 3, not inclusive.\n");
     printf("The suggested value is 2.75\n");
     fracdim = 0;
     while ((fracdim <= 2) || (fracdim >=3))
     {
       get_double("Dimension is : ",0,&fracdim);
     }
    } else { fracdim += 3.0; }
   }
   fracdim = 3.0 - fracdim;
   if (conf_flag != 2)
   {
    printf("\nPlease enter the percentage of water you wish in your\n");
    printf("world.  The value must be between 0 and 100.\n");
    percent = -1;
    while ((percent < 0 ) || (percent > 100))
    {
      get_int("Percentage of water: ",1,&percent);
    }
    perc_d = (double)percent/100.0;
   }
   gen_alt(&world,perc_d,fracdim,lacun,info_flag,fp);
   if (info_flag == 3)
   {
     conf_flag = -1;
     while ((conf_flag < 1) || (conf_flag > 3))
     {
       get_int("1) Ok Proceed 2) Redo w/same 3) Reenter stats : ",
                 3,&conf_flag);
     }
   } else { conf_flag = 1; }
  }

  conf_flag = 0;
  if (info_flag == 1)
  {
    fracdim = 0.7;
    gen_climate(fracdim,lacun,&world,info_flag,fp);
  } else
  {
   while (conf_flag != 1)
   {
    if (conf_flag != 2)
    {
      printf("\nThe climate will be fractally distributed.  The entered\n");
      printf("dimension is a value between 2 and 3 not inclusive.  A \n");
      printf("suggested value would be 2.3 \n");
      fracdim = 0;
      while ((fracdim <= 2) || (fracdim >=3))
      {
        get_double("Dimension is : ",0,&fracdim);
      }
      fracdim = 3.0 - fracdim;
    }
    gen_climate(fracdim,lacun,&world,info_flag,fp);
    if (info_flag == 3)
    {
      conf_flag = -1;
      while ((conf_flag < 1) || (conf_flag > 3))
      {
        get_int("1) Ok Proceed 2) Redo w/same 3) Reenter stats : ",
                  3,&conf_flag);
      }
    } else { conf_flag = 1; }
   }
  }
  adjust_terrain(&world);

  conf_flag = 0;
  if (info_flag == 1)
  {
    perc_d = 1.0 - 0.15;
    average = 4.5;
    gen_metal(&world,perc_d,average,info_flag,fp);
  } else 
  {
   while (conf_flag != 1)
   {
    if (conf_flag != 2)
    {
      printf("\nPlease enter the percentage of sectors with metal, and the\n");
      printf("average value for those sectors. The suggested values are\n");
      printf("15 percent and an avererage of 4.5\n");
      percent = -1;
      while ((percent < 0 ) || (percent > 100))
      {
        get_int("Perctage of sectors with metal: ",1,&percent);
      }
      perc_d = 1.0 - ((double)percent/100.0);
      average = -1.0;
      while (average < 0.0 )
      {
        get_double("Average value of metal sector: ",-1,&average);
      }
    }
    gen_metal(&world,perc_d,average,info_flag,fp);
    if (info_flag == 3)
    {
      conf_flag = -1;
      while ((conf_flag < 1) || (conf_flag > 3))
      {
        get_int("1) Ok Proceed 2) Redo w/same 3) Reenter stats : ",
                  3,&conf_flag);
      }
    } else { conf_flag = 1; }
   }
  }

  conf_flag = 0;
  if (info_flag == 1)
  {
    perc_d = 1.0 - 0.12;
    average = 4.0;
    gen_jewel(&world,perc_d, average,info_flag,fp);
  } else
  {
   while (conf_flag != 1)
   {
    if (conf_flag != 2)
    {
      printf("\nPlease enter the percentage of sectors with jewels, and\n");
      printf("the average value for those sectors. The suggested values\n");
      printf("are 12 percent and an avererage of 4.0\n");
      percent = -1;
      while ((percent < 0 ) || (percent > 100))
      {
        get_int("Perctage of sectors with jewels: ",1,&percent);
      }
      perc_d = 1.0 - ((double)percent/100.0);
      average = -1.0;
      while (average < 0.0 )
      {
        get_double("Average value of jewel sector: ",-1,&average);
      }
    }
    gen_jewel(&world,perc_d, average,info_flag,fp);
    if (info_flag == 3)
    {
      conf_flag = -1;
      while ((conf_flag < 1) || (conf_flag > 3))
      {
        get_int("1) Ok Proceed 2) Redo w/same 3) Reenter stats : ",
                  3,&conf_flag);
      }
    } else { conf_flag = 1; }
   }
  }

  conf_flag = 0;
  if (info_flag == 1)
  {
    average = 5.0;
    gen_soil(&world,fracdim,lacun,average,info_flag,fp);
  } else
  {
   while (conf_flag != 1)
   {
    if (conf_flag != 2)
    {
      printf("\nPlease enter the value for the average soil value\n");
      printf("5.0 is the suggested average.  Be careful with soil\n");
      average = -1.0;
      while (average < 0.0 )
      {
        get_double("Richness of soil: ",-1,&average);
      }
    }
    gen_soil(&world,fracdim,lacun,average,info_flag,fp);
    if (info_flag == 3)
    {
      conf_flag = -1;
      while ((conf_flag < 1) || (conf_flag > 3))
      {
        get_int("1) Ok Proceed 2) Redo w/same 3) Reenter stats : ",
                  3,&conf_flag);
      }
    } else { conf_flag = 1; }
   }
  }
  init_gamemaster(world.nations);
  world.n_nations = 1;
/*  world.geo.depth = 2;
  world.geo.sides = 1;
*/
  fclose (fp);
  set_compressed();
  write_world(&world, WORLD_FILE);
  set_update_time();
  exit(0);
}

void clean_exit(){};

