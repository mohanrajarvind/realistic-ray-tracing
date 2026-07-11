#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <random>
#include <memory>
#include <algorithm>

float randomFloat(){ static std::mt19937 g(1234567); static std::uniform_real_distribution<float> d(0,1); return d(g); }
float randomFloat(float a,float b){ return a+(b-a)*randomFloat(); }


class Vector3 {
public:
    float x,y,z;
    Vector3(){x=0;y=0;z=0;}
    Vector3(float a,float b,float c){x=a;y=b;z=c;}
    Vector3 operator+(const Vector3&v)const{return Vector3(x+v.x,y+v.y,z+v.z);}
    Vector3 operator-(const Vector3&v)const{return Vector3(x-v.x,y-v.y,z-v.z);}
    Vector3 operator*(float k)const{return Vector3(x*k,y*k,z*k);}
    Vector3 operator*(const Vector3&v)const{return Vector3(x*v.x,y*v.y,z*v.z);}
    Vector3 operator/(float k)const{return Vector3(x/k,y/k,z/k);}
    float operator[](int i)const{return i==0?x:(i==1?y:z);}
    float lengthSq()const{return x*x+y*y+z*z;}
    float length()const{return std::sqrt(lengthSq());}
    bool nearZero()const{float s=1e-8f;return std::fabs(x)<s&&std::fabs(y)<s&&std::fabs(z)<s;}
};
Vector3 operator*(float k,const Vector3&v){return v*k;}
float dot(const Vector3&a,const Vector3&b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Vector3 cross(const Vector3&a,const Vector3&b){return Vector3(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);}
Vector3 normalize(const Vector3&v){return v/v.length();}
Vector3 reflect(const Vector3&v,const Vector3&n){return v-2*dot(v,n)*n;}
Vector3 refract(const Vector3&uv,const Vector3&n,float er){
    float ct=std::fmin(dot(uv*-1.0f,n),1.0f); Vector3 perp=er*(uv+ct*n);
    Vector3 par=n*-std::sqrt(std::fabs(1.0f-perp.lengthSq())); return perp+par;
}
float reflectance(float c,float ri){float r0=(1-ri)/(1+ri);r0=r0*r0;return r0+(1-r0)*std::pow(1-c,5);}
Vector3 randomUnitVector(){for(int i=0;i<100;i++){Vector3 p(randomFloat(-1,1),randomFloat(-1,1),randomFloat(-1,1));if(p.lengthSq()<1)return normalize(p);}return Vector3(0,1,0);}
Vector3 randomInUnitDisk(){for(int i=0;i<100;i++){Vector3 p(randomFloat(-1,1),randomFloat(-1,1),0);if(p.lengthSq()<1)return p;}return Vector3(0,0,0);}
const float PI=3.14159265f;

class rgb {
public:
    float r,g,b;
    rgb(){r=0;g=0;b=0;}
    rgb(float a,float b_,float c){r=a;g=b_;b=c;}
    rgb operator+(const rgb&c)const{return rgb(r+c.r,g+c.g,b+c.b);}
    rgb operator*(float k)const{return rgb(r*k,g*k,b*k);}
    rgb operator*(const rgb&c)const{return rgb(r*c.r,g*c.g,b*c.b);}
    rgb operator/(float k)const{return rgb(r/k,g/k,b/k);}
};
int toByte(float x){x=std::sqrt(x);if(x<0)x=0;if(x>1)x=1;return int(255.99f*x);}

class Perlin {
public:
    static const int SIZE=256; float ran[SIZE]; int px[SIZE],py[SIZE],pz[SIZE];
    Perlin(){for(int i=0;i<SIZE;i++)ran[i]=randomFloat();perm(px);perm(py);perm(pz);}
    void perm(int*p){for(int i=0;i<SIZE;i++)p[i]=i;for(int i=SIZE-1;i>0;i--){int t=int(randomFloat()*(i+1));std::swap(p[i],p[t]);}}
    static float fade(float t){return t*t*(3-2*t);}
    float noise(const Vector3&pp)const{
        float u=pp.x-std::floor(pp.x),v=pp.y-std::floor(pp.y),w=pp.z-std::floor(pp.z);
        int i=int(std::floor(pp.x))&255,j=int(std::floor(pp.y))&255,k=int(std::floor(pp.z))&255;
        float c[2][2][2];
        for(int di=0;di<2;di++)for(int dj=0;dj<2;dj++)for(int dk=0;dk<2;dk++)
            c[di][dj][dk]=ran[px[(i+di)&255]^py[(j+dj)&255]^pz[(k+dk)&255]];
        float uu=fade(u),vv=fade(v),ww=fade(w),res=0;
        for(int di=0;di<2;di++)for(int dj=0;dj<2;dj++)for(int dk=0;dk<2;dk++){
            float wi=di?uu:1-uu,wj=dj?vv:1-vv,wk=dk?ww:1-ww;
            res+=wi*wj*wk*c[di][dj][dk];
        }
        return res;
    }
};
Perlin g_perlin;
float turbulence(const Vector3&p,int oct){
    float sum=0,w=1; Vector3 t=p;
    for(int i=0;i<oct;i++){ sum+=w*std::fabs(g_perlin.noise(t)*2-1); w*=0.5f; t=t*2; }
    return sum;
}

class Texture { public: virtual rgb value(const Vector3&p)const=0; virtual ~Texture(){} };
class Solid : public Texture { public: rgb c; Solid(rgb c_){c=c_;} rgb value(const Vector3&)const override{return c;} };
class Checker : public Texture { public: rgb a,b; float f; Checker(rgb a_,rgb b_,float f_){a=a_;b=b_;f=f_;}
    rgb value(const Vector3&p)const override{ float s=std::sin(f*p.x)*std::sin(f*p.z); return s<0?a:b; } };
class Marble : public Texture { public: rgb base,vein;
    Marble(rgb b_,rgb v_){base=b_;vein=v_;}
    rgb value(const Vector3&p)const override{ float t=0.5f*(1+std::sin(3.0f*p.y+8.0f*turbulence(p,5))); return base*t+vein*(1-t); } };

class Ray { public: Vector3 origin,direction; Ray(){} Ray(const Vector3&o,const Vector3&d){origin=o;direction=d;} Vector3 pointAt(float t)const{return origin+direction*t;} };
class Camera {
public:
    Vector3 origin,ll,horiz,vert,u,v,w; float lensRadius;
    Camera(Vector3 lf,Vector3 la,Vector3 vup,float fov,float asp,float aperture,float focus){
        lensRadius=aperture/2;
        float th=fov*PI/180,h=std::tan(th/2),vh=2*h,vw=asp*vh;
        w=normalize(lf-la); u=normalize(cross(vup,w)); v=cross(w,u);
        origin=lf; horiz=focus*vw*u; vert=focus*vh*v;
        ll=origin-horiz*0.5f-vert*0.5f-focus*w;
    }
    Ray getRay(float s,float t)const{
        Vector3 rd=lensRadius*randomInUnitDisk(); Vector3 off=u*rd.x+v*rd.y;
        return Ray(origin+off, ll+s*horiz+t*vert-origin-off);
    }
};

class Material; struct HitRecord { float t; Vector3 point,normal; bool frontFace; std::shared_ptr<Material> mat; };
class Material { public: virtual bool scatter(const Ray&,const HitRecord&,rgb&,Ray&)const=0; virtual ~Material(){} };

class Lambertian : public Material {
public:
    std::shared_ptr<Texture> tex;
    Lambertian(std::shared_ptr<Texture> t){tex=t;}
    Lambertian(rgb c){tex=std::make_shared<Solid>(c);}
    bool scatter(const Ray&,const HitRecord&rec,rgb&att,Ray&sc)const override{
        Vector3 d=rec.normal+randomUnitVector(); if(d.nearZero())d=rec.normal;
        sc=Ray(rec.point,d); att=tex->value(rec.point); return true;
    }
};
class Metal : public Material {
public:
    rgb albedo; float fuzz; Metal(rgb a,float f){albedo=a;fuzz=f<1?f:1;}
    bool scatter(const Ray&in,const HitRecord&rec,rgb&att,Ray&sc)const override{
        Vector3 r=reflect(normalize(in.direction),rec.normal);
        sc=Ray(rec.point,r+fuzz*randomUnitVector()); att=albedo; return dot(sc.direction,rec.normal)>0;
    }
};
class Dielectric : public Material {
public:
    float ir; Dielectric(float i){ir=i;}
    bool scatter(const Ray&in,const HitRecord&rec,rgb&att,Ray&sc)const override{
        att=rgb(1,1,1); float ratio=rec.frontFace?(1.0f/ir):ir;
        Vector3 ud=normalize(in.direction); float ct=std::fmin(dot(ud*-1.0f,rec.normal),1.0f),st=std::sqrt(1-ct*ct);
        Vector3 d;
        if(ratio*st>1.0f||reflectance(ct,ratio)>randomFloat()) d=reflect(ud,rec.normal);
        else d=refract(ud,rec.normal,ratio);
        sc=Ray(rec.point,d); return true;
    }
};
class DiffuseSpecular : public Material {
public:
    rgb albedo; float spec,fuzz; DiffuseSpecular(rgb a,float s,float f){albedo=a;spec=s;fuzz=f;}
    bool scatter(const Ray&in,const HitRecord&rec,rgb&att,Ray&sc)const override{
        if(randomFloat()<spec){ Vector3 r=reflect(normalize(in.direction),rec.normal);
            sc=Ray(rec.point,r+fuzz*randomUnitVector()); att=rgb(1,1,1); return dot(sc.direction,rec.normal)>0; }
        Vector3 d=rec.normal+randomUnitVector(); if(d.nearZero())d=rec.normal;
        sc=Ray(rec.point,d); att=albedo; return true;
    }
};

class AABB {
public:
    Vector3 lo,hi; AABB(){} AABB(Vector3 a,Vector3 b){lo=a;hi=b;}
    bool hit(const Ray&ray,float tmin,float tmax)const{
        for(int a=0;a<3;a++){ float invD=1.0f/ray.direction[a];
            float t0=(lo[a]-ray.origin[a])*invD, t1=(hi[a]-ray.origin[a])*invD;
            if(invD<0){float tmp=t0;t0=t1;t1=tmp;}
            if(t0>tmin)tmin=t0; if(t1<tmax)tmax=t1; if(tmax<=tmin)return false; }
        return true;
    }
};
AABB surrounding(const AABB&a,const AABB&b){
    return AABB(Vector3(std::fmin(a.lo.x,b.lo.x),std::fmin(a.lo.y,b.lo.y),std::fmin(a.lo.z,b.lo.z)),
               Vector3(std::fmax(a.hi.x,b.hi.x),std::fmax(a.hi.y,b.hi.y),std::fmax(a.hi.z,b.hi.z)));
}
class Hittable { public: virtual bool hit(const Ray&,float,float,HitRecord&)const=0; virtual AABB box()const=0; virtual ~Hittable(){} };

class Sphere : public Hittable {
public:
    Vector3 center; float radius; std::shared_ptr<Material> mat;
    Sphere(Vector3 c,float r,std::shared_ptr<Material> m){center=c;radius=r;mat=m;}
    bool hit(const Ray&ray,float tmin,float tmax,HitRecord&rec)const override{
        Vector3 oc=ray.origin-center; float a=ray.direction.lengthSq(),hb=dot(oc,ray.direction),c=oc.lengthSq()-radius*radius;
        float disc=hb*hb-a*c; if(disc<0)return false; float sq=std::sqrt(disc),root=(-hb-sq)/a;
        if(root<tmin||root>tmax){root=(-hb+sq)/a;if(root<tmin||root>tmax)return false;}
        rec.t=root; rec.point=ray.pointAt(root); rec.mat=mat;
        Vector3 outn=normalize(rec.point-center); rec.frontFace=dot(ray.direction,outn)<0;
        rec.normal=rec.frontFace?outn:outn*-1.0f; return true;
    }
    AABB box()const override{return AABB(center-Vector3(radius,radius,radius),center+Vector3(radius,radius,radius));}
};

class BVHNode : public Hittable {
public:
    std::shared_ptr<Hittable> left,right; AABB nb;
    BVHNode(std::vector<std::shared_ptr<Hittable>>&o,size_t s,size_t e){
        int axis=int(randomFloat()*3); size_t span=e-s;
        auto cmp=[axis](const std::shared_ptr<Hittable>&a,const std::shared_ptr<Hittable>&b){return a->box().lo[axis]<b->box().lo[axis];};
        if(span==1){left=right=o[s];}
        else if(span==2){left=o[s];right=o[s+1];}
        else{ std::sort(o.begin()+s,o.begin()+e,cmp); size_t m=s+span/2;
            left=std::make_shared<BVHNode>(o,s,m); right=std::make_shared<BVHNode>(o,m,e); }
        nb=surrounding(left->box(),right->box());
    }
    bool hit(const Ray&ray,float tmin,float tmax,HitRecord&rec)const override{
        if(!nb.hit(ray,tmin,tmax))return false;
        bool hl=left->hit(ray,tmin,tmax,rec); bool hr=right->hit(ray,tmin,hl?rec.t:tmax,rec);
        return hl||hr;
    }
    AABB box()const override{return nb;}
};

rgb trace(const Ray&ray,const Hittable&world,int depth){
    if(depth<=0)return rgb(0,0,0);
    HitRecord rec;
    if(world.hit(ray,0.001f,1e30f,rec)){
        Ray sc; rgb att;
        if(rec.mat->scatter(ray,rec,att,sc)) return att*trace(sc,world,depth-1);
        return rgb(0,0,0);
    }
    Vector3 u=normalize(ray.direction); float t=0.5f*(u.y+1.0f);   // sky = the big soft light
    return rgb(1,1,1)*(1.0f-t)+rgb(0.5f,0.7f,1.0f)*t;
}

int main(){
    std::vector<std::shared_ptr<Hittable>> objs;

    auto floorTex=std::make_shared<Checker>(rgb(0.2f,0.3f,0.15f),rgb(0.9f,0.9f,0.9f),3.0f);
    objs.push_back(std::make_shared<Sphere>(Vector3(0,-1000,0),1000, std::make_shared<Lambertian>(floorTex)));

    objs.push_back(std::make_shared<Sphere>(Vector3(-3,1,0),1.0f, std::make_shared<Dielectric>(1.5f)));       // glass
    objs.push_back(std::make_shared<Sphere>(Vector3(-1,1,0),1.0f,
        std::make_shared<Lambertian>(std::make_shared<Marble>(rgb(0.9f,0.9f,0.95f),rgb(0.15f,0.05f,0.2f))))); // marble
    objs.push_back(std::make_shared<Sphere>(Vector3(1,1,0),1.0f, std::make_shared<Metal>(rgb(0.8f,0.7f,0.4f),0.0f))); // gold mirror
    objs.push_back(std::make_shared<Sphere>(Vector3(3,1,0),1.0f, std::make_shared<DiffuseSpecular>(rgb(0.7f,0.15f,0.15f),0.2f,0.0f))); // red plastic

    for(int a=-6;a<6;a++)for(int b=-6;b<6;b++){
        Vector3 c(a+0.9f*randomFloat(),0.2f,b+0.9f*randomFloat());

        bool skip=false; float hx[4]={-3,-1,1,3};
        for(float h:hx) if((c-Vector3(h,0.2f,0)).length()<1.4f) skip=true;
        if((c-Vector3(0,0.2f,0)).length()<1.2f) skip=true;
        if(skip)continue;
        float ch=randomFloat();
        std::shared_ptr<Material> m;
        if(ch<0.7f) m=std::make_shared<Lambertian>(rgb(randomFloat()*randomFloat(),randomFloat()*randomFloat(),randomFloat()*randomFloat()));
        else if(ch<0.9f) m=std::make_shared<Metal>(rgb(randomFloat(0.5f,1),randomFloat(0.5f,1),randomFloat(0.5f,1)),randomFloat(0,0.4f));
        else m=std::make_shared<Dielectric>(1.5f);
        objs.push_back(std::make_shared<Sphere>(c,0.2f,m));
    }

    std::cout<<"Scene has "<<objs.size()<<" objects. Building BVH...\n";
    BVHNode world(objs,0,objs.size());

    int width=600,height=340,samples=80,maxDepth=25;
    float asp=float(width)/float(height);
    Vector3 lookFrom(10,2.2f,9),lookAt(0,0.8f,0);
    Camera cam(lookFrom,lookAt,Vector3(0,1,0),26.0f,asp,0.12f,(lookFrom-lookAt).length());

    std::vector<rgb> pix(width*height);
    std::cout<<"Rendering "<<width<<"x"<<height<<" at "<<samples<<" samples (this is the big one)...\n";
    for(int j=0;j<height;j++){
        for(int i=0;i<width;i++){
            rgb col(0,0,0);
            for(int s=0;s<samples;s++){
                float u=(i+randomFloat())/(width-1),v=(j+randomFloat())/(height-1);
                col=col+trace(cam.getRay(u,v),world,maxDepth);
            }
            pix[j*width+i]=col/float(samples);
        }
        if(j%20==0)std::cout<<"row "<<j<<"/"<<height<<"\n";
    }

    std::ofstream o("grand_finale.ppm"); o<<"P3\n"<<width<<" "<<height<<"\n255\n";
    for(int j=height-1;j>=0;j--)for(int i=0;i<width;i++){rgb c=pix[j*width+i];o<<toByte(c.r)<<" "<<toByte(c.g)<<" "<<toByte(c.b)<<"\n";}
    std::cout<<"Wrote grand_finale.ppm\n";
    return 0;
}
