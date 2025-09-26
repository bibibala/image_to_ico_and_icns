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

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

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

// ---------------- ICNS 生成 ----------------
static int convertToICNS(const char* input,const char* output){
    int w,h,ch;
    unsigned char* img=stbi_load(input,&w,&h,&ch,4);
    if(!img) return -1;

    FILE* out=fopen(output,"wb");
    if(!out){stbi_image_free(img);return -2;}

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
    stbi_image_free(img);
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
    int w,h,ch; unsigned char* img=stbi_load(input,&w,&h,&ch,4);
    if(!img) return -1;

    // 更新了尺寸数组，包含新增的尺寸
    int sizes[]={16,24,30,32,40,48,64,72,80,96,128,256,512,1024};
    int n=sizeof(sizes)/sizeof(sizes[0]);

    ico_hdr_t hdr={0,1,(uint16_t)n};
    ico_dir_t* dirs=(ico_dir_t*)calloc(n,sizeof(ico_dir_t));
    if(!dirs){stbi_image_free(img); return -2;}

    FILE* out=fopen(output,"wb");
    if(!out){stbi_image_free(img); free(dirs); return -3;}

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
    fclose(out); free(dirs); stbi_image_free(img);
    return 0;
}

// ---------------- PNG 多尺寸 ----------------
EMSCRIPTEN_KEEPALIVE
int wasm_convert_to_pngs(const char* input){
    int w,h,ch; unsigned char* img=stbi_load(input,&w,&h,&ch,4);
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

    stbi_image_free(img);
    printf("Successfully generated %d out of %d PNG files\n", success_count, n);
    return success_count == n ? 0 : -1;
}

// ---------------- 导出接口 ----------------
EMSCRIPTEN_KEEPALIVE
int wasm_convert_to_icns(const char* input,const char* output){ return convertToICNS(input,output); }
EMSCRIPTEN_KEEPALIVE
int wasm_convert_to_ico(const char* input,const char* output){ return convertToICO(input,output); }
EMSCRIPTEN_KEEPALIVE
int wasm_convert_to_both(const char* input,const char* prefix){
    char icns[256],ico[256];
    snprintf(icns,sizeof(icns),"%s.icns",prefix);
    snprintf(ico,sizeof(ico),"%s.ico",prefix);

    int r1=convertToICNS(input,icns);
    int r2=convertToICO(input,ico);
    wasm_convert_to_pngs(input); // 生成所有 PNG
    return (r1==0 && r2==0)?0:-1;
}
