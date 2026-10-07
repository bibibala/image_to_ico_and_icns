#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

// ---------------- ICNS ----------------
typedef struct { char magic[4]; uint32_t length; } icns_header_t;
typedef struct { char type[4]; uint32_t length; } icns_entry_header_t;
typedef struct { const char* type; int size; } icns_size_map_t;

static const icns_size_map_t icns_sizes[] = {
    {"ic10",1024},{"ic09",512},{"ic14",512},{"ic08",256},{"ic13",256},
    {"ic07",128},{"ic12",64},{"ic11",32},{"ic05",64},{"ic04",32},
    {"ic06",48},{"ic03",16},{NULL,0}
};

static uint32_t to_big_endian(uint32_t v){
    return ((v&0xFF)<<24)|(((v>>8)&0xFF)<<16)|(((v>>16)&0xFF)<<8)|(v>>24&0xFF);
}

// ---------------- PNG 内存写入 ----------------
typedef struct { unsigned char* data; size_t size; size_t cap; } mem_buf_t;
static void png_write_func(void* ctx, void* data, int sz){
    mem_buf_t* buf=(mem_buf_t*)ctx;
    if(buf->size+sz>buf->cap){
        size_t n=(buf->size+sz)*2;
        buf->data=(unsigned char*)realloc(buf->data,n);
        buf->cap=n;
    }
    memcpy(buf->data+buf->size,data,sz);
    buf->size+=sz;
}

static unsigned char* create_png_mem(unsigned char* img,int w,int h,int comp,size_t* out_size){
    mem_buf_t buf={malloc(4096),0,4096};
    stbi_write_png_to_func(png_write_func,&buf,w,h,comp,img,w*comp);
    *out_size=buf.size;
    return buf.data;
}

// ---------------- 通用格式 ----------------
typedef enum { FMT_UNKNOWN=0, FMT_PNG, FMT_JPG, FMT_BMP, FMT_TGA, FMT_HDR, FMT_ICO, FMT_ICNS } img_fmt_t;

#define SVG_MAX_DIM 8192

static int str_ieq(const char* a,const char* b){
    while(*a&&*b){
        char ca=*a++,cb=*b++;
        if(ca>='A'&&ca<='Z')ca=(char)(ca+32);
        if(cb>='A'&&cb<='Z')cb=(char)(cb+32);
        if(ca!=cb) return 0;
    }
    return *a==*b;
}

static const char* file_ext(const char* path){
    const char* base=strrchr(path,'/');
    base=base?base+1:path;
    const char* dot=strrchr(base,'.');
    return dot?dot+1:NULL;
}

static img_fmt_t fmt_from_path(const char* path){
    const char* e=file_ext(path);
    if(!e) return FMT_UNKNOWN;
    if(str_ieq(e,"png")) return FMT_PNG;
    if(str_ieq(e,"jpg")||str_ieq(e,"jpeg")) return FMT_JPG;
    if(str_ieq(e,"bmp")) return FMT_BMP;
    if(str_ieq(e,"tga")) return FMT_TGA;
    if(str_ieq(e,"hdr")) return FMT_HDR;
    if(str_ieq(e,"ico")) return FMT_ICO;
    if(str_ieq(e,"icns")) return FMT_ICNS;
    return FMT_UNKNOWN;
}

static int is_svg_file(const char* path){
    const char* e=file_ext(path);
    if(e&&str_ieq(e,"svg")) return 1;
    FILE* f=fopen(path,"rb");
    if(!f) return 0;
    char buf[1025];
    size_t n=fread(buf,1,1024,f);
    fclose(f);
    buf[n>1024?1024:n]='\0';
    return strstr(buf,"<svg")!=NULL;
}

static unsigned char* load_svg_rgba(const char* input,int req_w,int req_h,int* out_w,int* out_h){
    NSVGimage* svg=nsvgParseFromFile(input,"px",96.0f);
    if(!svg) return NULL;
    float nw=svg->width>0?svg->width:1.0f;
    float nh=svg->height>0?svg->height:1.0f;
    int w,h;
    if(req_w>0&&req_h>0){ w=req_w; h=req_h; }
    else if(req_w>0){ w=req_w; h=(int)(nh*(float)req_w/nw+0.5f); }
    else if(req_h>0){ h=req_h; w=(int)(nw*(float)req_h/nh+0.5f); }
    else{ w=(int)(nw+0.5f); h=(int)(nh+0.5f); }
    if(w<1)w=1; if(h<1)h=1;
    if(w>SVG_MAX_DIM||h>SVG_MAX_DIM){
        if(w>=h){ h=(int)((float)h*SVG_MAX_DIM/w); w=SVG_MAX_DIM; }
        else{ w=(int)((float)w*SVG_MAX_DIM/h); h=SVG_MAX_DIM; }
    }
    unsigned char* buf=(unsigned char*)calloc((size_t)w*h,4);
    if(!buf){ nsvgDelete(svg); return NULL; }
    NSVGrasterizer* rast=nsvgCreateRasterizer();
    if(!rast){ free(buf); nsvgDelete(svg); return NULL; }
    float scale=(float)w/nw;
    nsvgRasterize(rast,svg,0.0f,0.0f,scale,buf,w,h,w*4);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(svg);
    *out_w=w; *out_h=h;
    return buf;
}

static unsigned char* load_any_rgba(const char* input,int* w,int* h){
    if(is_svg_file(input)) return load_svg_rgba(input,0,0,w,h);
    int ch;
    return stbi_load(input,w,h,&ch,4);
}

static int img_has_alpha(const unsigned char* img,int npx){
    for(int i=0;i<npx;i++)
        if(img[i*4+3]!=255) return 1;
    return 0;
}

static void compute_ratio(int w,int h,int max_size,int* nw,int* nh){
    *nw=w; *nh=h;
    if(max_size>0&&(w>max_size||h>max_size)){
        if(w>=h){ *nw=max_size; *nh=(int)((float)h*max_size/w+0.5f); }
        else{ *nh=max_size; *nw=(int)((float)w*max_size/h+0.5f); }
    }
}

static int write_image8(const char* output,const unsigned char* img,int w,int h,int comp,int quality){
    img_fmt_t fmt=fmt_from_path(output);
    if(quality<1||quality>100) quality=85;
    if(comp==4&&!img_has_alpha(img,w*h)) comp=3;
    switch(fmt){
    case FMT_PNG:
        stbi_write_png_compression_level=(quality*9)/100;
        return stbi_write_png(output,w,h,comp,img,w*comp)?0:-5;
    case FMT_JPG:
        return stbi_write_jpg(output,w,h,comp,img,quality)?0:-5;
    case FMT_BMP:
        return stbi_write_bmp(output,w,h,comp,img)?0:-5;
    case FMT_TGA:
        return stbi_write_tga(output,w,h,comp,img)?0:-5;
    default:
        return -3;
    }
}

// ---------------- ICNS 生成 ----------------
static int convertToICNS(const char* input,const char* output){
    int w,h;
    unsigned char* img=load_any_rgba(input,&w,&h);
    if(!img) return -1;

    FILE* out=fopen(output,"wb");
    if(!out){free(img);return -2;}

    icns_header_t hdr; memcpy(hdr.magic,"icns",4); hdr.length=0;
    fwrite(&hdr,sizeof(hdr),1,out);
    size_t total_size=sizeof(hdr);

    for(int i=0;icns_sizes[i].type;i++){
        int size=icns_sizes[i].size;
        unsigned char* resized=(unsigned char*)malloc(size*size*4);
        if(!resized) continue;
        if(!stbir_resize_uint8(img,w,h,0,resized,size,size,0,4)){ free(resized); continue; }

        size_t png_sz; unsigned char* png=create_png_mem(resized,size,size,4,&png_sz);
        free(resized); if(!png) continue;

        icns_entry_header_t entry; memcpy(entry.type,icns_sizes[i].type,4);
        entry.length=to_big_endian((uint32_t)(sizeof(entry)+png_sz));
        fwrite(&entry,sizeof(entry),1,out);
        fwrite(png,1,png_sz,out);
        total_size+=sizeof(entry)+png_sz;
        free(png);
    }

    fseek(out,4,SEEK_SET);
    uint32_t be=to_big_endian((uint32_t)total_size);
    fwrite(&be,sizeof(be),1,out);
    fclose(out);
    free(img);
    return 0;
}

// ---------------- ICO 生成 ----------------
#pragma pack(push,1)
typedef struct{uint16_t reserved; uint16_t type; uint16_t count;} ico_hdr_t;
typedef struct{
    uint8_t width,height,colors,reserved;
    uint16_t planes,bitcount;
    uint32_t size,offset;
} ico_dir_t;
#pragma pack(pop)

static int convertToICO(const char* input,const char* output){
    int w,h; unsigned char* img=load_any_rgba(input,&w,&h);
    if(!img) return -1;

    // 更新了尺寸数组，包含新增的尺寸
    int sizes[]={16,24,30,32,40,48,64,72,80,96,128,256,512,1024};
    int n=sizeof(sizes)/sizeof(sizes[0]);

    ico_hdr_t hdr={0,1,(uint16_t)n};
    ico_dir_t* dirs=(ico_dir_t*)calloc(n,sizeof(ico_dir_t));
    if(!dirs){free(img); return -2;}

    FILE* out=fopen(output,"wb");
    if(!out){free(img); free(dirs); return -3;}

    fwrite(&hdr,sizeof(hdr),1,out);
    long dir_pos=ftell(out); fseek(out,sizeof(ico_dir_t)*n,SEEK_CUR);

    for(int i=0;i<n;i++){
        int size=sizes[i];
        unsigned char* resized=(unsigned char*)malloc(size*size*4);
        stbir_resize_uint8(img,w,h,0,resized,size,size,0,4);

        size_t png_sz; unsigned char* png=create_png_mem(resized,size,size,4,&png_sz);
        free(resized);

        long offset=ftell(out);
        fwrite(png,1,png_sz,out);
        free(png);

        dirs[i].width=(size==256||size==1024)?0:size;
        dirs[i].height=(size==256||size==1024)?0:size;
        dirs[i].colors=0; dirs[i].reserved=0;
        dirs[i].planes=1; dirs[i].bitcount=32;
        dirs[i].size=(uint32_t)png_sz;
        dirs[i].offset=(uint32_t)offset;
    }

    fseek(out,dir_pos,SEEK_SET);
    fwrite(dirs,sizeof(ico_dir_t),n,out);
    fclose(out); free(dirs); free(img);
    return 0;
}

// ---------------- PNG 多尺寸 ----------------
int wasm_convert_to_pngs(const char* input){
    int w,h; unsigned char* img=load_any_rgba(input,&w,&h);
    if(!img) return -1;

    // 更新了尺寸数组，包含新增的尺寸
    int sizes[]={16,24,30,32,40,48,64,72,80,96,128,256,512,1024};
    int n=sizeof(sizes)/sizeof(sizes[0]);
    int success_count = 0;

    for(int i=0;i<n;i++){
        int sz=sizes[i];
        unsigned char* resized=(unsigned char*)malloc(sz*sz*4);
        if(!resized) {
            printf("Failed to allocate memory for size %d\n", sz);
            continue;
        }

        int resize_result = stbir_resize_uint8(img,w,h,0,resized,sz,sz,0,4);
        if(!resize_result) {
            printf("Failed to resize to %dx%d\n", sz, sz);
            free(resized);
            continue;
        }

        size_t png_sz;
        unsigned char* png=create_png_mem(resized,sz,sz,4,&png_sz);
        free(resized);
        if(!png) {
            printf("Failed to create PNG for size %d\n", sz);
            continue;
        }

        char fname[32];
        snprintf(fname,sizeof(fname),"/%d.png", sz);
        FILE* out=fopen(fname,"wb");
        if(out){
            size_t written = fwrite(png,1,png_sz,out);
            fclose(out);
            if(written == png_sz) {
                success_count++;
                printf("Successfully created %s (%zu bytes)\n", fname, png_sz);
            } else {
                printf("Failed to write complete file %s\n", fname);
            }
        } else {
            printf("Failed to open file %s for writing\n", fname);
        }
        free(png);
    }

    free(img);
    printf("Successfully generated %d out of %d PNG files\n", success_count, n);
    return success_count == n ? 0 : -1;
}

// ---------------- 导出接口 ----------------
int wasm_convert_to_icns(const char* input,const char* output){ return convertToICNS(input,output); }
int wasm_convert_to_ico(const char* input,const char* output){ return convertToICO(input,output); }
int wasm_convert_to_both(const char* input,const char* prefix){
    char icns[256],ico[256];
    snprintf(icns,sizeof(icns),"%s.icns",prefix);
    snprintf(ico,sizeof(ico),"%s.ico",prefix);

    int r1=convertToICNS(input,icns);
    int r2=convertToICO(input,ico);
    wasm_convert_to_pngs(input); // 生成所有 PNG
    return (r1==0 && r2==0)?0:-1;
}

// SVG → png/jpg/bmp/tga，width/height<=0 时用原始尺寸，可只指定一边按比例缩放
int wasm_svg_to_image(const char* input,const char* output,int width,int height,int quality){
    img_fmt_t fmt=fmt_from_path(output);
    if(fmt==FMT_UNKNOWN||fmt==FMT_HDR||fmt==FMT_ICO||fmt==FMT_ICNS) return -3;
    int w,h;
    unsigned char* img=load_svg_rgba(input,width,height,&w,&h);
    if(!img) return -5;
    int r=write_image8(output,img,w,h,4,quality);
    free(img);
    return r;
}

// 压缩/重新编码：quality 1-100，jpg 为质量、png/bmp/tga 为压缩强度；max_size>0 时按最长边等比缩小
int wasm_compress_image(const char* input,const char* output,int quality,int max_size){
    img_fmt_t fmt=fmt_from_path(output);
    if(fmt==FMT_UNKNOWN) return -3;
    if(fmt==FMT_ICO||fmt==FMT_ICNS) return -3;

    if(fmt==FMT_HDR){
        int w,h,ch;
        float* img=stbi_loadf(input,&w,&h,&ch,4);
        if(!img) return -1;
        int nw,nh; compute_ratio(w,h,max_size,&nw,&nh);
        if(nw!=w||nh!=h){
            float* out=(float*)malloc((size_t)nw*nh*4*sizeof(float));
            if(!out){ stbi_image_free(img); return -4; }
            if(!stbir_resize_float(img,w,h,0,out,nw,nh,0,4)){ free(out); stbi_image_free(img); return -4; }
            stbi_image_free(img); img=out; w=nw; h=nh;
        }
        int ok=stbi_write_hdr(output,w,h,4,img);
        stbi_image_free(img);
        return ok?0:-5;
    }

    int w,h;
    unsigned char* img=load_any_rgba(input,&w,&h);
    if(!img) return -1;
    int nw,nh; compute_ratio(w,h,max_size,&nw,&nh);
    if(nw!=w||nh!=h){
        unsigned char* out=(unsigned char*)malloc((size_t)nw*nh*4);
        if(!out){ free(img); return -4; }
        if(!stbir_resize_uint8(img,w,h,0,out,nw,nh,0,4)){ free(out); free(img); return -4; }
        free(img); img=out; w=nw; h=nh;
    }
    int r=write_image8(output,img,w,h,4,quality);
    free(img);
    return r;
}

// 任意格式互转（输入可为常见位图或 SVG，按输出扩展名决定格式）
int wasm_convert_format(const char* input,const char* output,int quality){
    img_fmt_t fmt=fmt_from_path(output);
    if(fmt==FMT_UNKNOWN) return -3;
    if(fmt==FMT_ICO){
        if(quality>=1&&quality<=100) stbi_write_png_compression_level=(quality*9)/100;
        return convertToICO(input,output);
    }
    if(fmt==FMT_ICNS){
        if(quality>=1&&quality<=100) stbi_write_png_compression_level=(quality*9)/100;
        return convertToICNS(input,output);
    }
    return wasm_compress_image(input,output,quality,0);
}
