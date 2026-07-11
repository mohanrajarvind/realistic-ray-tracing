#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <random>
#include <memory>

float rnd(){ static std::mt19937 g(99); static std::uniform_real_distribution<float> d(0,1); return d(g); }

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
V3 randUnit(){for(int i=0;i<100;i++){V3 p(rnd()*2-1,rnd()*2-1,rnd()*2-1);if(p.lenSq()<1)return unit(p);}return V3(0,1,0);}
int toB(float x){x=std::sqrt(x);if(x<0)x=0;if(x>1)x=1;return int(255.99f*x);}

class Mat; struct Rec{float t;V3 p,n;bool front;std::shared_ptr<Mat> m;};
class Ray{public:V3 o,d;Ray(){}Ray(V3 a,V3 b){o=a;d=b;}V3 at(float t)const{return o+d*t;}};
class Mat{public:virtual bool scatter(const Ray&,const Rec&,V3&,Ray&)const=0;virtual ~Mat(){}};

class Lambert:public Mat{public:V3 a;Lambert(V3 c){a=c;}
    bool scatter(const Ray&,const Rec&r,V3&at,Ray&s)const override{
        V3 d=r.n+randUnit(); if(d.nearZero())d=r.n; s=Ray(r.p,d); at=a; return true;}};
class Metal:public Mat{public:V3 a;float f;Metal(V3 c,float fz){a=c;f=fz;}
    bool scatter(const Ray&in,const Rec&r,V3&at,Ray&s)const override{
        V3 rf=reflect(unit(in.d),r.n); s=Ray(r.p,rf+f*randUnit()); at=a; return dot(s.d,r.n)>0;}};
class Glass:public Mat{public:float ir;Glass(float i){ir=i;}
    bool scatter(const Ray&in,const Rec&r,V3&at,Ray&s)const override{
        at=V3(1,1,1); float ratio=r.front?(1/ir):ir; V3 ud=unit(in.d);
        float ct=std::fmin(dot(ud*-1.0f,r.n),1.0f),st=std::sqrt(1-ct*ct); V3 d;
        if(ratio*st>1||schlick(ct,ratio)>rnd()) d=reflect(ud,r.n); else d=refract(ud,r.n,ratio);
        s=Ray(r.p,d); return true;}};

class Sph{public:V3 c;float r;std::shared_ptr<Mat> m;
    Sph(V3 cc,float rr,std::shared_ptr<Mat> mm){c=cc;r=rr;m=mm;}
    bool hit(const Ray&ray,float tmin,float tmax,Rec&rec)const{
        V3 oc=ray.o-c; float a=ray.d.lenSq(),hb=dot(oc,ray.d),cc=oc.lenSq()-r*r;
        float dsc=hb*hb-a*cc; if(dsc<0)return false; float sq=std::sqrt(dsc),root=(-hb-sq)/a;
        if(root<tmin||root>tmax){root=(-hb+sq)/a;if(root<tmin||root>tmax)return false;}
        rec.t=root;rec.p=ray.at(root);rec.m=m;
        V3 on=unit(rec.p-c); rec.front=dot(ray.d,on)<0; rec.n=rec.front?on:on*-1.0f; return true;}};

std::vector<Sph> world;
bool hitW(const Ray&ray,Rec&rec){bool any=false;float cl=1e30f;Rec t;
    for(const Sph&s:world) if(s.hit(ray,0.001f,cl,t)){any=true;cl=t.t;rec=t;} return any;}

V3 trace(const Ray&ray,int depth){
    if(depth<=0)return V3(0,0,0);
    Rec rec;
    if(hitW(ray,rec)){ Ray sc; V3 att;
        if(rec.m->scatter(ray,rec,att,sc)) return att*trace(sc,depth-1);
        return V3(0,0,0); }
    V3 u=unit(ray.d); float t=0.5f*(u.y+1);
    return V3(1,1,1)*(1-t)+V3(0.5f,0.7f,1.0f)*t;
}

int main(){
    world.push_back(Sph(V3(0,-100.5f,-1),100, std::make_shared<Lambert>(V3(0.55f,0.55f,0.5f))));
    world.push_back(Sph(V3(-1.05f,0,-1),0.5f, std::make_shared<Glass>(1.5f)));
    world.push_back(Sph(V3(-1.05f,0,-1),-0.45f, std::make_shared<Glass>(1.5f)));
    world.push_back(Sph(V3(0,0,-1),0.5f, std::make_shared<Lambert>(V3(0.75f,0.25f,0.25f))));
    world.push_back(Sph(V3(1.05f,0,-1),0.5f, std::make_shared<Metal>(V3(0.85f,0.75f,0.45f),0.02f)));

    int W=700,H=400,S=150,D=30;
    float asp=float(W)/H;
    V3 lf(0,0.35f,2.0f), la(0,0,-1);
    float th=48.0f*3.14159265f/180.0f,h=std::tan(th/2),vh=2*h,vw=asp*vh;
    V3 w=unit(lf-la),u=unit(cross(V3(0,1,0),w)),v=cross(w,u);
    V3 horiz=vw*u, vert=vh*v, ll=lf-horiz*0.5f-vert*0.5f-w;

    std::ofstream out("post_2_materials.ppm"); out<<"P3\n"<<W<<" "<<H<<"\n255\n";
    std::vector<V3> px(W*H);
    for(int j=0;j<H;j++){
        for(int i=0;i<W;i++){
            V3 c(0,0,0);
            for(int s=0;s<S;s++){
                float su=(i+rnd())/(W-1), sv=(j+rnd())/(H-1);
                c=c+trace(Ray(lf, ll+su*horiz+sv*vert-lf), D);
            }
            px[j*W+i]=c/float(S);
        }
        if(j%40==0)std::cout<<"row "<<j<<"/"<<H<<"\n";
    }
    for(int j=H-1;j>=0;j--)for(int i=0;i<W;i++){V3 c=px[j*W+i];out<<toB(c.x)<<" "<<toB(c.y)<<" "<<toB(c.z)<<"\n";}
    std::cout<<"Wrote post_2_materials.ppm\n";
    return 0;
}
