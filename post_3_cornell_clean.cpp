#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <random>
#include <memory>

static thread_local std::mt19937 g_rng(2024);
float rnd(){ static std::uniform_real_distribution<float> d(0.0f,1.0f); return d(g_rng); }

class V3 { public: float x,y,z;
    V3(){x=0;y=0;z=0;} V3(float a,float b,float c){x=a;y=b;z=c;}
    V3 operator+(const V3&v)const{return V3(x+v.x,y+v.y,z+v.z);}
    V3 operator-(const V3&v)const{return V3(x-v.x,y-v.y,z-v.z);}
    V3 operator*(float k)const{return V3(x*k,y*k,z*k);}
    V3 operator*(const V3&v)const{return V3(x*v.x,y*v.y,z*v.z);}
    V3 operator/(float k)const{return V3(x/k,y/k,z/k);}
    float lenSq()const{return x*x+y*y+z*z;} float len()const{return std::sqrt(lenSq());}
    bool nearZero()const{return std::fabs(x)<1e-8f&&std::fabs(y)<1e-8f&&std::fabs(z)<1e-8f;} };
V3 operator*(float k,const V3&v){return v*k;}
float dot(const V3&a,const V3&b){return a.x*b.x+a.y*b.y+a.z*b.z;}
V3 cross(const V3&a,const V3&b){return V3(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);}
V3 unit(const V3&v){return v/v.len();}
V3 reflect(const V3&v,const V3&n){return v-2*dot(v,n)*n;}
V3 refract(const V3&uv,const V3&n,float er){float ct=std::fmin(dot(uv*-1.0f,n),1.0f);
    V3 pp=er*(uv+ct*n); V3 pl=n*-std::sqrt(std::fabs(1-pp.lenSq())); return pp+pl;}
float schlick(float c,float ri){float r0=(1-ri)/(1+ri);r0=r0*r0;return r0+(1-r0)*std::pow(1-c,5);}
const float PI=3.14159265f;

V3 cosineHemisphere(const V3&n){
    float r1=rnd(), r2=rnd();
    float phi=2*PI*r1, r=std::sqrt(r2), z=std::sqrt(1-r2);
    V3 a = (std::fabs(n.x)>0.9f)? V3(0,1,0) : V3(1,0,0);
    V3 t = unit(cross(a,n)); V3 b = cross(n,t);
    return unit(t*(r*std::cos(phi)) + b*(r*std::sin(phi)) + n*z);
}
V3 randUnit(){for(int i=0;i<200;i++){V3 p(rnd()*2-1,rnd()*2-1,rnd()*2-1);if(p.lenSq()<1&&p.lenSq()>1e-6f)return unit(p);}return V3(0,1,0);}

float aces(float x){
    float a=2.51f,b=0.03f,c=2.43f,d=0.59f,e=0.14f;
    float v=(x*(a*x+b))/(x*(c*x+d)+e);
    return v<0?0:(v>1?1:v);
}
int toB(float x){ x=aces(x); x=std::pow(x,1.0f/2.2f); return int(255.99f*(x<0?0:(x>1?1:x))); }

enum MatType { DIFFUSE, MIRROR, GLASS, EMIT };
struct Material { MatType type; V3 albedo; V3 emission; float ir;
    Material(MatType t,V3 a,V3 e=V3(0,0,0),float i=1.5f){type=t;albedo=a;emission=e;ir=i;} };

struct Rec { float t; V3 p,n; bool front; const Material* m; };
class Ray{public:V3 o,d;Ray(){}Ray(V3 a,V3 b){o=a;d=b;}V3 at(float t)const{return o+d*t;}};


struct Sphere { V3 c; float r; const Material* m;
    bool hit(const Ray&ray,float tmin,float tmax,Rec&rec)const{
        V3 oc=ray.o-c; float a=ray.d.lenSq(),hb=dot(oc,ray.d),cc=oc.lenSq()-r*r;
        float dsc=hb*hb-a*cc; if(dsc<0)return false;
        float sq=std::sqrt(dsc),root=(-hb-sq)/a;
        if(root<tmin||root>tmax){root=(-hb+sq)/a;if(root<tmin||root>tmax)return false;}
        rec.t=root;rec.p=ray.at(root);rec.m=m;
        V3 on=unit(rec.p-c); rec.front=dot(ray.d,on)<0; rec.n=rec.front?on:on*-1.0f; return true;} };

struct Quad { V3 corner,u,v,n; float area; const Material* m;
    void init(){ V3 nn=cross(u,v); area=nn.len(); n=unit(nn); }
    V3 randomPoint()const{ return corner + u*rnd() + v*rnd(); }
    bool hit(const Ray&ray,float tmin,float tmax,Rec&rec)const{
        float den=dot(ray.d,n); if(std::fabs(den)<1e-9f)return false;
        float t=dot(corner-ray.o,n)/den; if(t<tmin||t>tmax)return false;
        V3 p=ray.at(t), rel=p-corner, nn=cross(u,v), w=nn/dot(nn,nn);
        float a=dot(w,cross(rel,v)), b=dot(w,cross(u,rel));
        if(a<0||a>1||b<0||b>1)return false;
        rec.t=t;rec.p=p;rec.m=m;
        rec.front=dot(ray.d,n)<0; rec.n=rec.front?n:n*-1.0f; return true;} };

std::vector<Sphere> spheres;
std::vector<Quad> quads;
int lightQuad = -1;

bool hitScene(const Ray&ray,Rec&rec){
    bool any=false; float cl=1e30f; Rec t;
    for(const auto&s:spheres) if(s.hit(ray,0.001f,cl,t)){any=true;cl=t.t;rec=t;}
    for(const auto&q:quads)   if(q.hit(ray,0.001f,cl,t)){any=true;cl=t.t;rec=t;}
    return any;
}

bool occluded(const V3&p,const V3&target){
    V3 to=target-p; float dist=to.len(); V3 d=to/dist;
    Ray r(p,d); Rec rec;
    if(!hitScene(r,rec)) return false;
    return rec.t < dist-0.01f;      
}


V3 sampleLight(const Rec&rec){
    const Quad& L = quads[lightQuad];
    V3 lp = L.randomPoint();
    V3 to = lp - rec.p; float dist2 = to.lenSq(), dist = std::sqrt(dist2);
    V3 wi = to/dist;

    float cosSurf = dot(rec.n, wi);           if(cosSurf<=0) return V3(0,0,0);
    float cosLight= dot(L.n*-1.0f, wi);       if(cosLight<=0) return V3(0,0,0);
    if(occluded(rec.p, lp)) return V3(0,0,0);  

    V3 brdf = rec.m->albedo / PI;              
    float G = (cosSurf*cosLight)/dist2;        
    return brdf * L.m->emission * (G * L.area);
}

V3 trace(const Ray&ray,int depth,bool seeLight){
    if(depth<=0) return V3(0,0,0);
    Rec rec;
    if(!hitScene(ray,rec)) return V3(0,0,0);

    const Material& m = *rec.m;

    if(m.type==EMIT)
        return seeLight ? m.emission : V3(0,0,0);   

    if(m.type==MIRROR){
        V3 d = reflect(unit(ray.d), rec.n);
        return m.albedo * trace(Ray(rec.p,d), depth-1, true);
    }
    if(m.type==GLASS){
        float ratio = rec.front ? (1.0f/m.ir) : m.ir;
        V3 ud = unit(ray.d);
        float ct=std::fmin(dot(ud*-1.0f,rec.n),1.0f), st=std::sqrt(1-ct*ct);
        V3 d;
        if(ratio*st>1.0f || schlick(ct,ratio)>rnd()) d=reflect(ud,rec.n);
        else d=refract(ud,rec.n,ratio);
        return trace(Ray(rec.p,d), depth-1, true);
    }


    V3 direct = sampleLight(rec);
    V3 wi = cosineHemisphere(rec.n);                       
    V3 indirect = m.albedo * trace(Ray(rec.p,wi), depth-1, false);
    return direct + indirect;
}

int main(){

    static Material white(DIFFUSE, V3(0.73f,0.73f,0.73f));
    static Material red  (DIFFUSE, V3(0.65f,0.05f,0.05f));
    static Material green(DIFFUSE, V3(0.12f,0.45f,0.15f));
    static Material lampM(EMIT,    V3(0,0,0), V3(22,20,18));   
    static Material mirrorM(MIRROR,V3(0.92f,0.92f,0.94f));
    static Material glassM (GLASS, V3(1,1,1), V3(0,0,0), 1.52f);

    float S=555;
    Quad q;
    q={V3(S,0,0),V3(0,0,S),V3(0,S,0),V3(),0,&green};  q.init(); quads.push_back(q);  
    q={V3(0,0,0),V3(0,0,S),V3(0,S,0),V3(),0,&red};    q.init(); quads.push_back(q);  
    q={V3(0,0,0),V3(S,0,0),V3(0,0,S),V3(),0,&white};  q.init(); quads.push_back(q);  
    q={V3(0,S,0),V3(S,0,0),V3(0,0,S),V3(),0,&white};  q.init(); quads.push_back(q);  
    q={V3(0,0,S),V3(S,0,0),V3(0,S,0),V3(),0,&white};  q.init(); quads.push_back(q);  
    
    q={V3(160,S-0.5f,160),V3(235,0,0),V3(0,0,235),V3(),0,&lampM}; q.init();
    lightQuad=(int)quads.size(); quads.push_back(q);

    spheres.push_back({V3(180,120,200),120,&mirrorM});   
    spheres.push_back({V3(390,95,300), 95,&glassM});     

    int W=600,H=600,SPP=1024,D=16;      
    int sq=(int)std::sqrt((double)SPP); 
    V3 lf(278,278,-800), la(278,278,0);
    float th=40.0f*PI/180,h=std::tan(th/2),vh=2*h,vw=vh;
    V3 w=unit(lf-la),u=unit(cross(V3(0,1,0),w)),v=cross(w,u);
    V3 horiz=vw*u, vert=vh*v, ll=lf-horiz*0.5f-vert*0.5f-w;

    std::vector<V3> px(W*H);
    std::cout<<"Rendering clean Cornell Box "<<W<<"x"<<H<<" @ "<<sq*sq<<" spp\n";
    std::cout<<"(direct lighting + stratified sampling + tone mapping)\n";

    for(int j=0;j<H;j++){
        for(int i=0;i<W;i++){
            V3 c(0,0,0);
            
            for(int sy=0;sy<sq;sy++) for(int sx=0;sx<sq;sx++){
                float su=(i + (sx+rnd())/sq)/(W-1);
                float sv=(j + (sy+rnd())/sq)/(H-1);
                c=c+trace(Ray(lf, ll+su*horiz+sv*vert-lf), D, true);
            }
            px[j*W+i]=c/float(sq*sq);
        }
        if(j%25==0)std::cout<<"row "<<j<<"/"<<H<<"\n";
    }

    std::ofstream out("post_3_cornell_clean.ppm");
    out<<"P3\n"<<W<<" "<<H<<"\n255\n";
    for(int j=H-1;j>=0;j--)for(int i=0;i<W;i++){V3 c=px[j*W+i];out<<toB(c.x)<<" "<<toB(c.y)<<" "<<toB(c.z)<<"\n";}
    std::cout<<"Wrote post_3_cornell_clean.ppm\n";
    return 0;
}
