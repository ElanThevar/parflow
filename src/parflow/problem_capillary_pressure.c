/*BHEADER**********************************************************************
 * (c) 1995   The Regents of the University of California
 *
 * See the file COPYRIGHT_and_DISCLAIMER for a complete copyright
 * notice, contact person, and disclaimer.
 *
 * $Revision: 1.1.1.1 $
 *********************************************************************EHEADER*/

#include "parflow.h"

/*--------------------------------------------------------------------------
 * Structures
 *--------------------------------------------------------------------------*/

typedef struct
{
   int    num_phases;

   int   *type;
   void **data;

} PublicXtra;

typedef void InstanceXtra;

typedef struct
{
   NameArray regions;
   int      num_regions;

   int     *region_indices;
   double  *values;

} Type0;                       /* constant regions */


/*--------------------------------------------------------------------------
 * CapillaryPressure
 *--------------------------------------------------------------------------*/

void         CapillaryPressure(capillary_pressure, phase_i, phase_j,
			       problem_data, phase_saturation)
Vector      *capillary_pressure;
int          phase_i;
int          phase_j;
ProblemData *problem_data;
Vector      *phase_saturation;
{
   PFModule      *this_module   = ThisPFModule;
   PublicXtra    *public_xtra   = PFModulePublicXtra(this_module);

   Grid          *grid     = VectorGrid(capillary_pressure);

   Type0         *dummy0;

   SubgridArray  *subgrids = GridSubgrids(grid);

   Subgrid       *subgrid;

   Subvector     *cp_sub;
   Subvector     *ps_sub;

   double        *cpp;
   double        *psp;

   int            ix, iy, iz;
   int            nx, ny, nz;
   int            r;

   int            is, i, j, k, icp, ips;


   InitVector(capillary_pressure, 0.0);

   if (phase_i == phase_j)
      return;

   switch((public_xtra -> type[phase_i]))
   {
   case 0:
   {
      int      num_regions;
      int     *region_indices;
      double  *values;

      GrGeomSolid  *gr_solid;
      double        value;
      int           ir;


      dummy0 = (Type0 *)(public_xtra -> data[phase_i]);

      num_regions    = (dummy0 -> num_regions);
      region_indices = (dummy0 -> region_indices);
      values         = (dummy0 -> values);

      for (ir = 0; ir < num_regions; ir++)
      {
	 gr_solid = ProblemDataGrSolid(problem_data, region_indices[ir]);
	 value    = values[ir];

	 ForSubgridI(is, subgrids)
	 {
            subgrid   = SubgridArraySubgrid(subgrids, is);

	    cp_sub = VectorSubvector(capillary_pressure, is);
	    ps_sub = VectorSubvector(phase_saturation, is);

	    ix = SubgridIX(subgrid);
	    iy = SubgridIY(subgrid);
	    iz = SubgridIZ(subgrid);
	    
	    nx = SubgridNX(subgrid);
	    ny = SubgridNY(subgrid);
	    nz = SubgridNZ(subgrid);
	    
	    /* RDF: assume resolution is the same in all 3 directions */
	    r = SubgridRX(subgrid);
	    
	    cpp = SubvectorData(cp_sub);
	    psp = SubvectorData(ps_sub);
	    GrGeomInLoop(i, j, k, gr_solid, r, ix, iy, iz, nx, ny, nz,
            {
	       icp = SubvectorEltIndex(cp_sub, i, j, k);
	       ips = SubvectorEltIndex(ps_sub, i, j, k);

	       cpp[icp] = value * psp[ips];
	    });
	 }
      }

      break;
   }
   }
}


/*--------------------------------------------------------------------------
 * CapillaryPressureInitInstanceXtra
 *--------------------------------------------------------------------------*/

PFModule  *CapillaryPressureInitInstanceXtra()
{
   PFModule      *this_module   = ThisPFModule;
   InstanceXtra  *instance_xtra;

#if 0
   if ( PFModuleInstanceXtra(this_module) == NULL )
      instance_xtra = ctalloc(InstanceXtra, 1);
   else
      instance_xtra = PFModuleInstanceXtra(this_module);
#endif
   instance_xtra = NULL;

   PFModuleInstanceXtra(this_module) = instance_xtra;
   return this_module;
}


/*--------------------------------------------------------------------------
 * CapillaryPressureFreeInstanceXtra
 *--------------------------------------------------------------------------*/

void  CapillaryPressureFreeInstanceXtra()
{
   PFModule      *this_module   = ThisPFModule;
   InstanceXtra  *instance_xtra = PFModuleInstanceXtra(this_module);

   if (instance_xtra)
   {
      tfree(instance_xtra);
   }
}

/*--------------------------------------------------------------------------
 * CapillaryPressureNewPublicXtra
 *--------------------------------------------------------------------------*/

PFModule  *CapillaryPressureNewPublicXtra(num_phases)
int        num_phases;
{
   PFModule      *this_module   = ThisPFModule;
   PublicXtra    *public_xtra;

   Type0         *dummy0;

   int            i;

   char *phase_name;
   char *switch_name;
   char *region;
   char key[IDB_MAX_KEY_LEN];

   NameArray type_na;

   type_na = NA_NewNameArray("Constant");

   public_xtra = ctalloc(PublicXtra, 1);

   (public_xtra -> num_phases) = num_phases;

   (public_xtra -> type) = ctalloc(int,    num_phases);
   (public_xtra -> data) = ctalloc(void *, num_phases);

   for (i = 1; i < num_phases; i++)
   {

      phase_name = NA_IndexToName(GlobalsPhaseNames, i);

      sprintf(key, "CapPressure.%s.Type", phase_name);
      switch_name = GetStringDefault(key, "Constant");
      public_xtra -> type[i] = NA_NameToIndex(type_na, switch_name);

      switch((public_xtra -> type[i]))
      {
	 case 0:
	 {
	    int  num_regions, ir;
	    
	    
	    dummy0 = ctalloc(Type0, 1);

	    sprintf(key, "CapPressure.%s.GeomNames", phase_name);
	    switch_name = GetString(key);
	    dummy0 -> regions = NA_NewNameArray(switch_name);
	
	    dummy0 -> num_regions = NA_Sizeof(dummy0 -> regions);

	    num_regions = (dummy0 -> num_regions);
	    
	    (dummy0 -> region_indices) = ctalloc(int,    num_regions);
	    (dummy0 -> values)         = ctalloc(double, num_regions);

	    for (ir = 0; ir < num_regions; ir++)
	    {
	       region = NA_IndexToName(dummy0 -> regions, ir);

	       dummy0 -> region_indices[ir] = 
		  NA_NameToIndex(GlobalsGeomNames, region);

	       sprintf(key, "Geom.%s.CapPressure.%s.Value",
		       region, phase_name);
	       dummy0 -> values[ir] = GetDoubleDefault(key, 0.0);
	       
	    }
	    
	    (public_xtra -> data[i]) = (void *) dummy0;
	    
	    break;
	 }

	 default:
	 {
	    InputError("Error: invalid type <%s> for key <%s>\n",
		       switch_name, key);
	 }
      }
   }

   NA_FreeNameArray(type_na);

   PFModulePublicXtra(this_module) = public_xtra;
   return this_module;
}

/*-------------------------------------------------------------------------
 * CapillaryPressureFreePublicXtra
 *-------------------------------------------------------------------------*/

void  CapillaryPressureFreePublicXtra()
{
   PFModule    *this_module   = ThisPFModule;
   PublicXtra  *public_xtra   = PFModulePublicXtra(this_module);

   Type0       *dummy0;

   int          i;


   if (public_xtra)
   {
      for (i = 1; i < (public_xtra -> num_phases); i++)
      {
         switch((public_xtra -> type[i]))
	 {
	 case 0:
	 {
	    dummy0 = (Type0 *)(public_xtra -> data[i]);

	    NA_FreeNameArray(dummy0 -> regions);

	    tfree(dummy0 -> region_indices);
	    tfree(dummy0 -> values);
	    tfree(dummy0);
            break;
	 }
         }
      }

      tfree(public_xtra -> data);
      tfree(public_xtra -> type);

      tfree(public_xtra);
   }
}

/*--------------------------------------------------------------------------
 * CapillaryPressureSizeOfTempData
 *--------------------------------------------------------------------------*/

int  CapillaryPressureSizeOfTempData()
{
   return 0;
}
