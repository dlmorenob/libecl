/*
   Copyright (C) 2013  Statoil ASA, Norway. 
   
   The file 'well_segment.c' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

#include <stdbool.h>

#include <ert/util/util.h>
#include <ert/util/string_util.h>

#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/ecl_rsthead.h>

#include <ert/ecl_well/well_const.h>
#include <ert/ecl_well/well_conn.h>
#include <ert/ecl_well/well_segment.h>

#define WELL_SEGMENT_TYPE_ID  2209166 

struct well_segment_struct {
  UTIL_TYPE_ID_DECLARATION;
  int                 link_count;
  int                 segment_id;  
  int                 branch_id; 
  int                 outlet_segment_id;  // This is in the global index space given by the ISEG keyword.
  well_segment_type * outlet_segment;
  const double      * rseg_data;          // Shared data - owned by the RSEG keyword
};


UTIL_IS_INSTANCE_FUNCTION( well_segment , WELL_SEGMENT_TYPE_ID )
static UTIL_SAFE_CAST_FUNCTION( well_segment , WELL_SEGMENT_TYPE_ID )


well_segment_type * well_segment_alloc(int segment_id , int outlet_segment_id , int branch_id , const double * rseg_data) {
  well_segment_type * segment = util_malloc( sizeof * segment );
  UTIL_TYPE_ID_INIT( segment , WELL_SEGMENT_TYPE_ID );
  
  segment->link_count = 0;
  segment->segment_id = segment_id;
  segment->outlet_segment_id = outlet_segment_id;
  segment->branch_id = branch_id;
  segment->outlet_segment = NULL;
  segment->rseg_data = rseg_data;

  return segment;
}


well_segment_type * well_segment_alloc_from_kw( const ecl_kw_type * iseg_kw , const ecl_kw_type * rseg_kw , const ecl_rsthead_type * header , int well_nr, int segment_id) {
  const int iseg_offset = header->nisegz * ( header->nsegmx * well_nr + segment_id);
  const int rseg_offset = header->nrsegz * ( header->nsegmx * well_nr + segment_id);
  int outlet_segment_id = ecl_kw_iget_int( iseg_kw , iseg_offset + ISEG_OUTLET_ITEM );   
  int branch_id         = ecl_kw_iget_int( iseg_kw , iseg_offset + ISEG_BRANCH_ITEM );  
  const double * rseg_data = ecl_kw_iget_ptr( rseg_kw , rseg_offset );
  
  well_segment_type * segment = well_segment_alloc( segment_id , outlet_segment_id , branch_id , rseg_data);

  {
    int_vector_type * index_list = string_util_alloc_active_list("0,2-7,16-19,29-32,57,66-69,93");
    if (0) {
      printf("-----------------------------------------------------------------\n");
      for (int i=0;i < int_vector_size(index_list); i++) {
        int index = int_vector_iget( index_list , i );
        printf("%d:%g ",index , rseg_data[index]); 
      }
      printf("\n");
    }
    int_vector_free( index_list );
  }
  return segment;
}


/*
    if (iseg_kw != NULL) {
      if (conn->segment != WELL_CONN_NORMAL_WELL_SEGMENT_ID) {
  
      } else {
        conn->branch = 0;
        conn->outlet_segment = 0;
      }
    } else {
      conn->branch = 0;
      conn->outlet_segment = 0;
    }
    */


void well_segment_free(well_segment_type * segment ) {
  free( segment );
}

void well_segment_free__(void * arg) {
  well_segment_type * segment = well_segment_safe_cast( arg );
  well_segment_free( segment );
}


bool well_segment_active( const well_segment_type * segment ) {
  if (segment->branch_id == ECLIPSE_WELL_SEGMENT_BRANCH_INACTIVE_VALUE)
    return false;
  else
    return true;
}


bool well_segment_main_stem( const well_segment_type * segment ) {
  if (segment->branch_id == ECLIPSE_WELL_SEGMENT_BRANCH_MAIN_STEM_VALUE)
    return true;
  else
    return false;
}


bool well_segment_nearest_wellhead( const well_segment_type * segment ) {
  if (segment->outlet_segment_id == ECLIPSE_WELL_SEGMENT_OUTLET_END_VALUE)
    return true;
  else
    return false;
}
  

int well_segment_get_link_count( const well_segment_type * segment ) {
  return segment->link_count;
}

int well_segment_get_branch_id( const well_segment_type * segment ) {
  return segment->branch_id;
}

int well_segment_get_outlet_id( const well_segment_type * segment ) {
  return segment->outlet_segment_id;
}

int well_segment_get_id( const well_segment_type * segment ) {
  return segment->segment_id;
}


well_segment_type * well_segment_get_outlet( const well_segment_type * segment ) {
  return segment->outlet_segment;
}
  

bool well_segment_link( well_segment_type * segment , well_segment_type * outlet_segment ) {
  if (segment->outlet_segment_id == outlet_segment->segment_id) {
    segment->outlet_segment = outlet_segment;
    outlet_segment->link_count++;
    return true;
  } else 
    /* 
       This is a quite fatal topological error - and aborting is probaly the wisest
       thing to do. I.e.  the function well_segment_link_strict() is recommended.
    */
    return false;
}


void well_segment_link_strict( well_segment_type * segment , well_segment_type * outlet_segment ) {
  if (!well_segment_link( segment , outlet_segment))
    util_abort("%s: tried to create invalid link between segments %d and %d \n",segment->segment_id , outlet_segment->segment_id);
}


bool well_segment_add_connection( well_segment_type * segment , const char * grid_name , const well_conn_type * conn) {
  int conn_segment_id = well_conn_get_segment( conn );
  if (conn_segment_id == segment->segment_id) {
    
    return true;
  } else
    return false;  
}
