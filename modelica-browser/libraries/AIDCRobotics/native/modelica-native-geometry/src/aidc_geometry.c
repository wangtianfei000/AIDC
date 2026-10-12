#define AIDC_GEOMETRY_BUILD
#include "aidc_geometry.h"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <strings.h>
#define _stricmp strcasecmp
#endif

typedef struct { double lo[3], hi[3]; int begin, count, left, right; } Node;
typedef struct { double* triangles; int triangle_count; int* order; Node* nodes; int node_count; } Mesh;

static void vcopy(double d[3], const double s[3]) { d[0]=s[0]; d[1]=s[1]; d[2]=s[2]; }
static void vsub(double d[3], const double a[3], const double b[3]) { d[0]=a[0]-b[0]; d[1]=a[1]-b[1]; d[2]=a[2]-b[2]; }
static void vadd_scaled(double d[3], const double a[3], const double b[3], double s) { d[0]=a[0]+s*b[0]; d[1]=a[1]+s*b[1]; d[2]=a[2]+s*b[2]; }
static double dot(const double a[3], const double b[3]) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
static double norm2(const double a[3]) { return dot(a,a); }

static void transform(const double pose[7], const double p[3], double out[3]) {
  const double x=pose[3], y=pose[4], z=pose[5], w=pose[6];
  const double r00=1-2*(y*y+z*z), r01=2*(x*y-z*w), r02=2*(x*z+y*w);
  const double r10=2*(x*y+z*w), r11=1-2*(x*x+z*z), r12=2*(y*z-x*w);
  const double r20=2*(x*z-y*w), r21=2*(y*z+x*w), r22=1-2*(x*x+y*y);
  out[0]=pose[0]+r00*p[0]+r01*p[1]+r02*p[2];
  out[1]=pose[1]+r10*p[0]+r11*p[1]+r12*p[2];
  out[2]=pose[2]+r20*p[0]+r21*p[1]+r22*p[2];
}

static int append_triangle(double** data, int* count, int* capacity, const double p[9], const double scale[3]) {
  if (*count == *capacity) {
    int next=*capacity ? *capacity*2 : 32;
    double* resized=(double*)realloc(*data,(size_t)next*9*sizeof(double));
    if (!resized) return 0;
    *data=resized; *capacity=next;
  }
  for (int i=0;i<9;i++) (*data)[(*count)*9+i]=p[i]*scale[i%3];
  (*count)++;
  return 1;
}

static int load_obj(FILE* f, const double scale[3], double** triangles, int* count) {
  double* vertices=NULL; int vertex_count=0, vertex_capacity=0, capacity=0; char line[1024];
  while (fgets(line,sizeof(line),f)) {
    if (line[0]=='v' && (line[1]==' ' || line[1]=='\t')) {
      double x,y,z; if (sscanf(line+1,"%lf%lf%lf",&x,&y,&z)!=3) continue;
      if (vertex_count==vertex_capacity) { int next=vertex_capacity?vertex_capacity*2:64; double* p=(double*)realloc(vertices,(size_t)next*3*sizeof(double)); if(!p){free(vertices);return 0;} vertices=p;vertex_capacity=next; }
      vertices[3*vertex_count]=x;vertices[3*vertex_count+1]=y;vertices[3*vertex_count+2]=z;vertex_count++;
    } else if (line[0]=='f' && (line[1]==' ' || line[1]=='\t')) {
      int a,b,c; if (sscanf(line+1,"%d%*[^ ] %d%*[^ ] %d",&a,&b,&c)<3) { if(sscanf(line+1,"%d %d %d",&a,&b,&c)!=3) continue; }
      if(a<1||b<1||c<1||a>vertex_count||b>vertex_count||c>vertex_count) continue;
      double p[9]; memcpy(p,vertices+(a-1)*3,3*sizeof(double));memcpy(p+3,vertices+(b-1)*3,3*sizeof(double));memcpy(p+6,vertices+(c-1)*3,3*sizeof(double));
      if(!append_triangle(triangles,count,&capacity,p,scale)){free(vertices);return 0;}
    }
  }
  free(vertices); return *count>0;
}

static float read_f32(const unsigned char* p) { float v; memcpy(&v,p,4); return v; }
static int load_stl(FILE* f, const double scale[3], double** triangles, int* count) {
  unsigned char header[84]; int capacity=0;
  if(fread(header,1,84,f)!=84) return 0;
  uint32_t n; memcpy(&n,header+80,4);
  if(n>0 && n<10000000) {
    unsigned char record[50];
    for(uint32_t i=0;i<n;i++) { if(fread(record,1,50,f)!=50) return 0; double p[9]; for(int k=0;k<9;k++)p[k]=read_f32(record+12+4*k); if(!append_triangle(triangles,count,&capacity,p,scale))return 0; }
    return *count>0;
  }
  rewind(f); char line[512]; double p[9]; int vertex=0;
  while(fgets(line,sizeof(line),f)) if(strstr(line,"vertex")) { double x,y,z;if(sscanf(strstr(line,"vertex")+6,"%lf%lf%lf",&x,&y,&z)==3){p[3*vertex]=x;p[3*vertex+1]=y;p[3*vertex+2]=z;if(++vertex==3){if(!append_triangle(triangles,count,&capacity,p,scale))return 0;vertex=0;}} }
  return *count>0;
}

static void triangle_bounds(const Mesh* m, int triangle, double lo[3], double hi[3]) {
  const double* p=m->triangles+triangle*9;
  for(int a=0;a<3;a++){lo[a]=hi[a]=p[a];for(int v=1;v<3;v++){double x=p[v*3+a];if(x<lo[a])lo[a]=x;if(x>hi[a])hi[a]=x;}}
}
static double centroid_axis(const Mesh* m,int triangle,int axis){const double* p=m->triangles+triangle*9;return (p[axis]+p[3+axis]+p[6+axis])/3.0;}
static int build_node(Mesh* m,int begin,int count){
  int index=m->node_count++;Node* node=&m->nodes[index];node->begin=begin;node->count=count;node->left=node->right=-1;
  for(int a=0;a<3;a++){node->lo[a]=DBL_MAX;node->hi[a]=-DBL_MAX;}
  for(int i=begin;i<begin+count;i++){double lo[3],hi[3];triangle_bounds(m,m->order[i],lo,hi);for(int a=0;a<3;a++){if(lo[a]<node->lo[a])node->lo[a]=lo[a];if(hi[a]>node->hi[a])node->hi[a]=hi[a];}}
  if(count<=4)return index;
  double extent[3]={node->hi[0]-node->lo[0],node->hi[1]-node->lo[1],node->hi[2]-node->lo[2]};int axis=extent[1]>extent[0]?1:0;if(extent[2]>extent[axis])axis=2;
  for(int i=begin+1;i<begin+count;i++){int key=m->order[i],j=i-1;double key_value=centroid_axis(m,key,axis);while(j>=begin){double value=centroid_axis(m,m->order[j],axis);if(value<key_value||(value==key_value&&m->order[j]<key))break;m->order[j+1]=m->order[j];j--;}m->order[j+1]=key;}
  int left_count=count/2;node->left=build_node(m,begin,left_count);node->right=build_node(m,begin+left_count,count-left_count);return index;
}

static void closest_point_triangle(const double p[3],const double a[3],const double b[3],const double c[3],double q[3]) {
  double ab[3],ac[3],ap[3];vsub(ab,b,a);vsub(ac,c,a);vsub(ap,p,a);double d1=dot(ab,ap),d2=dot(ac,ap);if(d1<=0&&d2<=0){vcopy(q,a);return;}
  double bp[3];vsub(bp,p,b);double d3=dot(ab,bp),d4=dot(ac,bp);if(d3>=0&&d4<=d3){vcopy(q,b);return;}double vc=d1*d4-d3*d2;if(vc<=0&&d1>=0&&d3<=0){vadd_scaled(q,a,ab,d1/(d1-d3));return;}
  double cp[3];vsub(cp,p,c);double d5=dot(ab,cp),d6=dot(ac,cp);if(d6>=0&&d5<=d6){vcopy(q,c);return;}double vb=d5*d2-d1*d6;if(vb<=0&&d2>=0&&d6<=0){vadd_scaled(q,a,ac,d2/(d2-d6));return;}double va=d3*d6-d5*d4;if(va<=0&&(d4-d3)>=0&&(d5-d6)>=0){double bc[3];vsub(bc,c,b);vadd_scaled(q,b,bc,(d4-d3)/((d4-d3)+(d5-d6)));return;}double denom=1.0/(va+vb+vc);vadd_scaled(q,a,ab,vb*denom);vadd_scaled(q,q,ac,vc*denom);
}
static void world_triangle(const Mesh* m,int ti,const double pose[7],double p[3][3]){const double* t=m->triangles+ti*9;for(int i=0;i<3;i++)transform(pose,t+3*i,p[i]);}
static void test_pair(const Mesh* a,int ia,const double pa[7],const Mesh* b,int ib,const double pb[7],double* best,double outa[3],double outb[3]){
  double ta[3][3],tb[3][3];world_triangle(a,ia,pa,ta);world_triangle(b,ib,pb,tb);
  for(int i=0;i<3;i++){double q[3],d[3];closest_point_triangle(ta[i],tb[0],tb[1],tb[2],q);vsub(d,ta[i],q);double d2=norm2(d);if(d2<*best){*best=d2;vcopy(outa,ta[i]);vcopy(outb,q);}closest_point_triangle(tb[i],ta[0],ta[1],ta[2],q);vsub(d,q,tb[i]);d2=norm2(d);if(d2<*best){*best=d2;vcopy(outa,q);vcopy(outb,tb[i]);}}
}
static void world_bounds(const Mesh* m,const Node* n,const double pose[7],double lo[3],double hi[3]){for(int a=0;a<3;a++){lo[a]=DBL_MAX;hi[a]=-DBL_MAX;}for(int mask=0;mask<8;mask++){double p[3]={mask&1?n->hi[0]:n->lo[0],mask&2?n->hi[1]:n->lo[1],mask&4?n->hi[2]:n->lo[2]},q[3];transform(pose,p,q);for(int a=0;a<3;a++){if(q[a]<lo[a])lo[a]=q[a];if(q[a]>hi[a])hi[a]=q[a];}}}
static double bounds_distance2(const double alo[3],const double ahi[3],const double blo[3],const double bhi[3]){double d2=0;for(int a=0;a<3;a++){double d=ahi[a]<blo[a]?blo[a]-ahi[a]:bhi[a]<alo[a]?alo[a]-bhi[a]:0;d2+=d*d;}return d2;}
static void traverse(const Mesh* a,int na,const double pa[7],const Mesh* b,int nb,const double pb[7],double* best,double outa[3],double outb[3]){
  const Node *aa=&a->nodes[na],*bb=&b->nodes[nb];double alo[3],ahi[3],blo[3],bhi[3];world_bounds(a,aa,pa,alo,ahi);world_bounds(b,bb,pb,blo,bhi);if(bounds_distance2(alo,ahi,blo,bhi)>*best)return;
  if(aa->left<0&&bb->left<0){for(int i=aa->begin;i<aa->begin+aa->count;i++)for(int j=bb->begin;j<bb->begin+bb->count;j++)test_pair(a,a->order[i],pa,b,b->order[j],pb,best,outa,outb);return;}
  if(bb->left<0||(aa->left>=0&&aa->count>=bb->count)){traverse(a,aa->left,pa,b,nb,pb,best,outa,outb);traverse(a,aa->right,pa,b,nb,pb,best,outa,outb);}else{traverse(a,na,pa,b,bb->left,pb,best,outa,outb);traverse(a,na,pa,b,bb->right,pb,best,outa,outb);}
}

const char* aidc_geometry_version(void){return "0.1.0";}
int aidc_geometry_version_code(void){return 100;}
AidcMeshHandle aidc_mesh_load(const char* path,const double scale[3]){
  if(!path||!scale)return NULL;FILE* f=fopen(path,"rb");if(!f)return NULL;Mesh* m=(Mesh*)calloc(1,sizeof(Mesh));if(!m){fclose(f);return NULL;}const char* ext=strrchr(path,'.');int ok=ext&&(_stricmp(ext,".stl")==0)?load_stl(f,scale,&m->triangles,&m->triangle_count):load_obj(f,scale,&m->triangles,&m->triangle_count);fclose(f);if(!ok){aidc_mesh_destroy(m);return NULL;}m->order=(int*)malloc((size_t)m->triangle_count*sizeof(int));m->nodes=(Node*)calloc((size_t)m->triangle_count*2,sizeof(Node));if(!m->order||!m->nodes){aidc_mesh_destroy(m);return NULL;}for(int i=0;i<m->triangle_count;i++)m->order[i]=i;build_node(m,0,m->triangle_count);return m;
}
void aidc_mesh_destroy(AidcMeshHandle handle){Mesh* m=(Mesh*)handle;if(!m)return;free(m->triangles);free(m->order);free(m->nodes);free(m);}
int aidc_mesh_triangle_count(AidcMeshHandle handle){Mesh* m=(Mesh*)handle;return m?m->triangle_count:0;}
int aidc_mesh_bvh_node_count(AidcMeshHandle handle){Mesh* m=(Mesh*)handle;return m?m->node_count:0;}
int aidc_mesh_distance(AidcMeshHandle ha,const double pa[7],AidcMeshHandle hb,const double pb[7],double* distance,double point_a[3],double point_b[3],double normal[3]){
  Mesh *a=(Mesh*)ha,*b=(Mesh*)hb;if(!a||!b||!pa||!pb||!distance||!point_a||!point_b||!normal)return 1;double best=DBL_MAX;traverse(a,0,pa,b,0,pb,&best,point_a,point_b);if(best==DBL_MAX)return 2;*distance=sqrt(best);double d[3];vsub(d,point_b,point_a);double n=sqrt(norm2(d));if(n>1e-15){normal[0]=d[0]/n;normal[1]=d[1]/n;normal[2]=d[2]/n;}else{normal[0]=0;normal[1]=0;normal[2]=1;}return 0;
}
int aidc_mesh_contacts(AidcMeshHandle ha,const double pa[7],AidcMeshHandle hb,const double pb[7],int capacity,double out[]){
  Mesh *a=(Mesh*)ha,*b=(Mesh*)hb;if(!a||!b||capacity<0||(!out&&capacity))return -1;double alo[3],ahi[3],blo[3],bhi[3];world_bounds(a,&a->nodes[0],pa,alo,ahi);world_bounds(b,&b->nodes[0],pb,blo,bhi);double overlap[3];for(int i=0;i<3;i++){overlap[i]=fmin(ahi[i],bhi[i])-fmax(alo[i],blo[i]);if(overlap[i]<0)return 0;}if(capacity==0)return 1;int axis=overlap[1]<overlap[0]?1:0;if(overlap[2]<overlap[axis])axis=2;double ca=(alo[axis]+ahi[axis])/2,cb=(blo[axis]+bhi[axis])/2;for(int i=0;i<3;i++){out[i]=(fmax(alo[i],blo[i])+fmin(ahi[i],bhi[i]))/2;out[3+i]=0;}out[3+axis]=cb>=ca?1:-1;out[6]=overlap[axis];return 1;
}
