#if 0
bmp2png.exe: bmp2png.obj
 link /nologo /dynamicbase:no /ltcg /machine:x86 /map /merge:.rdata=.text /nxcompat /opt:icf /opt:ref /release /out:bmp2png.exe bmp2png.obj kernel32.lib user32.lib
 @echo.
 @-peman.exe /stsv bmp2png.exe 3.10 > nul
 @-peman.exe /vwsc bmp2png.exe

bmp2png.obj: bmp2png.cpp
 cl /nologo /c /GF /GL /GR- /GS- /Gy /MD /O1ib1 /W4 /WX /Zl /fp:fast /D "_STATIC_CPPLIB" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" bmp2png.cpp

!IF 0
#else

#define SUPPORT_RLE
//#define SUPPORT_BITFIELDS
#define TARGET_WIN32GUI
#define USE_OBSOLETEFILEAPI

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRTDBG_MAP_ALLOC

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdint.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <direct.h>
#include <io.h>
#include <share.h>

#define for         if (0); else for

#ifdef _MSC_VER
#define INLINE      __forceinline
#define NOINLINE    __declspec(noinline)
#else
#define INLINE      inline
#define NOINLINE
#endif


#if defined(_WINDOWS) && defined(TARGET_WIN32GUI)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#undef TARGET_WIN32GUI
#endif


#ifdef _MSC_VER
#include <tchar.h>
#include <crtdbg.h>
#include <intrin.h>

#ifdef TARGET_WIN32GUI
#pragma comment(linker, "/subsystem:windows")
#else
#pragma comment(linker, "/defaultlib:msvcrt.lib")
#pragma comment(linker, "/defaultlib:oldnames.lib")
#endif

#pragma warning(disable: 4127 4480)
#else
typedef char        _TCHAR;
#define _T(x)       (x)
#endif

#ifndef CharNext
inline _TCHAR* CharNext(const _TCHAR* p) { return const_cast<_TCHAR*>(*p ? p + 1 : p); }
#endif

#ifndef wsprintf
inline int wsprintf(_TCHAR* buff, const _TCHAR* format, ...)
{
    va_list argPtr;
    va_start(argPtr, format);
    return vsprintf(buff, format, argPtr);
}
#endif


#ifdef TARGET_WIN32GUI
template <typename T>
inline T* __cdecl MemAlloc(size_t num) throw()
{
    _ASSERT(SIZE_MAX / sizeof(T) >= num);
    return static_cast<T*>(::LocalAlloc(LPTR, sizeof(T) * num));
}
inline void __cdecl MemFree(__inout_opt void *p) throw()
{
    ::LocalFree(p);
}
#else
template <typename T>
inline T* __cdecl MemAlloc(size_t num) throw()
{
    _ASSERT(SIZE_MAX / sizeof(T) >= num);
    return static_cast<T*>(::calloc(num, sizeof(T)));
}
inline void __cdecl MemFree(__inout_opt void *p) throw()
{
    ::free(p);
}
#endif  //SUPPORT_WINALLOC

#ifdef _DEBUG
#define trace(...)              Debug::Trace(__VA_ARGS__)
#else
#define trace(...)
#endif

namespace Debug
{
void Trace(__in __format_string const char* format, ...);
}

inline void InlineCopyMemory(__out_bcount(cb) void* dst, __in_bcount(cb) const void* src, size_t cb)
{
#ifdef _MSC_VER
    __movsb(static_cast<unsigned char*>(dst), static_cast<const unsigned char*>(src), cb);
#else
    memcpy(dst, src, cb);
#endif
}

inline void InlineZeroMemoryDword(__out_bcount(cb) void* dst, size_t cb)
{
#ifdef _MSC_VER
    __stosd(static_cast<unsigned long*>(dst), 0, cb / 4);
#else
    memset(dst, 0, cb);
#endif
}

inline size_t InlineStrLen(const _TCHAR* s)
{
    size_t c = 0;
    for (; *s++; c++);
    return c;
}

inline void InlineStrCpy(_TCHAR* d, const _TCHAR* s)
{
    do {
        *d++ = *s;
    } while (*s++);
}

template <typename T> inline T Min(T x, T y) { return x < y ? x : y; }
template <typename T> inline T Max(T x, T y) { return x > y ? x : y; }


#ifdef TARGET_WIN32GUI

class CFileBase
{
private:
    HANDLE hf;

public:
    CFileBase() : hf(INVALID_HANDLE_VALUE) {}
    ~CFileBase() { Close(); }

    bool IsValidHandle() const { return this->hf != INVALID_HANDLE_VALUE; }

    bool Open(__in const _TCHAR* filename)
    {
        _ASSERT(IsValidHandle() == false);
        this->hf = ::CreateFile(filename, GENERIC_READ,
                                FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, NULL);
        return IsValidHandle();
    }

    bool Create(__in const _TCHAR* filename)
    {
        _ASSERT(IsValidHandle() == false);
        this->hf = ::CreateFile(filename, GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, NULL);
        return IsValidHandle();
    }

    bool CreateNew(__in const _TCHAR* filename)
    {
        _ASSERT(IsValidHandle() == false);
        this->hf = ::CreateFile(filename, GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ, NULL, CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL, NULL);
        return IsValidHandle();
    }

#ifdef USE_OBSOLETEFILEAPI
// HACK: the following code assumes that HFILEs and HANDLEs are interchangeable

    void Close()
    {
        if (IsValidHandle()) {
            ::_lclose(HandleToLong(this->hf));
            this->hf = INVALID_HANDLE_VALUE;
        }
    }

    INLINE bool Read(__out_bcount_part(cb, *pcbRead) void* pv, uint32_t cb)
    {
        _ASSERT(IsValidHandle());
        uint32_t cbRead = ::_lread(HandleToLong(this->hf), pv, cb);
        return (cbRead == cb);
    }

    INLINE bool Write(__in_bcount(cb) const void* pv, uint32_t cb)
    {
        _ASSERT(IsValidHandle());
        uint32_t cbWritten = ::_lwrite(HandleToLong(this->hf), static_cast<LPCSTR>(pv), cb);
        return (cbWritten == cb);
    }

    uint32_t Seek(int32_t offset, unsigned int method)
    {
        _ASSERT(IsValidHandle());
        _STATIC_ASSERT(FILE_BEGIN == SEEK_SET);
        _STATIC_ASSERT(FILE_CURRENT == SEEK_CUR);
        _STATIC_ASSERT(FILE_END == SEEK_END);
        return ::_llseek(HandleToLong(this->hf), offset, method);
    }

#else   // !USE_OBSOLETEFILEAPI

    void Close()
    {
        if (IsValidHandle()) {
            ::CloseHandle(this->hf);
            this->hf = INVALID_HANDLE_VALUE;
        }
    }

    INLINE bool Read(__out_bcount_part(cb, *pcbRead) void* pv, uint32_t cb)
    {
        _ASSERT(IsValidHandle());
        DWORD cbRead;
        return (::ReadFile(this->hf, pv, cb, &cbRead, NULL) && cbRead == cb);
    }

    INLINE bool Write(__in_bcount(cb) const void* pv, uint32_t cb)
    {
        _ASSERT(IsValidHandle());
        DWORD cbWritten;
        return (::WriteFile(this->hf, pv, cb, &cbWritten, NULL) && cbWritten == cb);
    }

    uint32_t Seek(int32_t offset, unsigned int method)
    {
        _ASSERT(IsValidHandle());
        _STATIC_ASSERT(FILE_BEGIN == SEEK_SET);
        _STATIC_ASSERT(FILE_CURRENT == SEEK_CUR);
        _STATIC_ASSERT(FILE_END == SEEK_END);
        return ::SetFilePointer(this->hf, offset, NULL, method);
    }

#endif
};

#else

class CFileBase
{
private:
    int fd;

public:
    CFileBase() : fd(-1) {}
    ~CFileBase() { Close(); }

    bool IsValidHandle() const { return this->fd != -1; }

    bool Open(__in const _TCHAR* filename)
    {
        _ASSERT(IsValidHandle() == false);
        this->fd = ::open(filename, O_BINARY | O_RDONLY, S_IREAD);
        return IsValidHandle();
    }

    bool Create(__in const _TCHAR* filename)
    {
        _ASSERT(IsValidHandle() == false);
        this->fd = ::open(filename, O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD);
        return IsValidHandle();
    }

    bool CreateNew(__in const _TCHAR* filename)
    {
        _ASSERT(IsValidHandle() == false);
        this->fd = ::open(filename, O_RDWR | O_BINARY | O_CREAT | O_EXCL, S_IREAD);
        return IsValidHandle();
    }

    void Close()
    {
        if (IsValidHandle()) {
            ::close(this->fd);
            this->fd = -1;
        }
    }

    bool Read(__out_bcount_part(cb, *pcbRead) void* pv, uint32_t cb)
    {
        _ASSERT(IsValidHandle());
        int cbRead = ::read(this->fd, pv, cb);
        return (cbRead == cb);
    }

    bool Write(__in_bcount(cb) const void* pv, uint32_t cb)
    {
        _ASSERT(IsValidHandle());
        int cbWritten = ::write(this->fd, pv, cb);
        return (cbWritten == cb);
    }

    uint32_t Seek(int32_t offset, unsigned int method)
    {
        _ASSERT(IsValidHandle());
        return ::lseek(this->fd, offset, method);
    }
};

#endif


class CFile : public CFileBase
{
private:
    typedef CFileBase   super;
    CFile(const CFile&);
    CFile& operator =(const CFile&);

public:
    CFile() {}
    ~CFile() {}

    NOINLINE bool Read(__out_bcount(cb) void* pv, uint32_t cb)
    {
        return super::Read(pv, cb);
    }

    template <typename T>
    bool Read(__out T* pT)
    {
        return Read(pT, sizeof(T));
    }

    NOINLINE bool Write(__in_bcount(cb) const void* pv, uint32_t cb)
    {
        return super::Write(pv, cb);
    }

    template <typename T>
    bool Write(__in const T* pT)
    {
        return Write(pT, sizeof(T));
    }
};


class CByteStream
{
private:
    mutable uint8_t*   cur;
    uint8_t* const     begin;
    uint8_t* const     end;

    CByteStream(const CByteStream&);
    CByteStream& operator =(const CByteStream&);

public:
    CByteStream(__inout_ecount(cb) const uint8_t* p, rsize_t cb)
        : cur(const_cast<uint8_t*>(p))
        , begin(const_cast<uint8_t*>(p))
        , end(const_cast<uint8_t*>(p) + cb)
    {
    }

    bool ValidatePos() const
    {
        return (this->cur < this->end);
    }
    rsize_t Tell() const
    {
        return this->cur - this->begin;
    }
    void Seek(__in ptrdiff_t relPos) const
    {
        // WARN: DANGER!! DANGER!!
        // Passing an untrusted value as the relPos parameter could possibly cause an integer overflow
        this->cur += relPos;
        // Currently, this method is only valid with relPos = 1
        _ASSERT(relPos == 1);
    }
    void SeekTo(__in rsize_t absPos)const
    {
        absPos = Min(static_cast<rsize_t>(this->end - this->begin), absPos);
        this->cur = this->begin + absPos;
    }
    bool Peek(__out uint8_t* p) const
    {
        if (this->cur < this->end) {
            *p = *this->cur;
            return true;
        }
        return false;
    }
    bool Read(__out uint8_t* p) const
    {
        if (this->cur < this->end) {
            *p = *this->cur++;
            return true;
        }
        return false;
    }
    bool Write(__in uint8_t b)
    {
        if (this->cur < this->end) {
            *this->cur++ = b;
            return true;
        }
        return false;
    }
};


#ifndef mmioFOURCC
#define mmioFOURCC(ch0, ch1, ch2, ch3)                      \
        ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |   \
        ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24))
#endif

#define DWORDALIGN(dw)      (((dw)+3)&~3)
#define WORDALIGN(w)        (((w)+1)&~1)
#define DIBWIDTHBYTES(w,b)  ((((w)*(b)+31)&(~31))/8)
#define PNGWIDTHBYTES(w,c)  (((w)*(c)+7)/8)
#define MAKEZLIBFLAG(f,t)   ((uint8_t)(((f)&1)|(((t)&3)<<1)))

#include <pshpack1.h>

namespace BMP
{

typedef enum tagBmpCOMPRESSION
{
    RGB             = 0,
    RLE8            = 1,
    RLE4            = 2,
    BITFIELDS       = 3
} COMPRESSION;

typedef struct tagBmpPalette
{
    uint8_t         Blue;
    uint8_t         Green;
    uint8_t         Red;
    uint8_t         Reserved;
} Palette;

typedef struct tagBmpInfoHeader {
    uint32_t        Size;
    int32_t         Width;
    int32_t         Height;
    uint16_t        Planes;
    uint16_t        BitCount;
    uint32_t        Compression;
    uint32_t        SizeImage;
    int32_t         XPelsPerMeter;
    int32_t         YPelsPerMeter;
    uint32_t        ClrUsed;
    uint32_t        ClrImportant;
} InfoHeader;

typedef struct tagBmpFileHeader {
    uint16_t        Type;
    uint32_t        Size;
    uint32_t        Reserved;
    uint32_t        OffBits;
} FileHeader;

// sizeof(BITMAPINFOHEADER)
const uint32_t SIZE_BITMAPINFO = 40;
// sizeof(BITMAPCOREHEADER)
const uint32_t SIZE_BITMAPCORE = 12;

}

namespace PNG
{

#define PNG_SignatureBytes      { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n' }

const unsigned int MAX_PALETTE = 256;

namespace cktype
{

const uint32_t IHDR         = mmioFOURCC('I', 'H', 'D', 'R');
const uint32_t PLTE         = mmioFOURCC('P', 'L', 'T', 'E');
const uint32_t IDAT         = mmioFOURCC('I', 'D', 'A', 'T');
const uint32_t tRNS         = mmioFOURCC('t', 'R', 'N', 'S');
const uint32_t IEND         = mmioFOURCC('I', 'E', 'N', 'D');

}

typedef enum tagPngCOLORTYPE
{
    TYPE_GRAYSCALE          = 0,
    TYPE_RGB                = 2,
    TYPE_PALETTE            = 3,
    TYPE_GRAYSCALE_ALPHA    = 4,
    TYPE_RGB_ALPHA          = 6,
} COLORTYPE;

typedef struct tagPngChunk
{
    uint32_t        cbSize;
    uint32_t        type;
} Chunk;

typedef struct tagPngHeader
{
    uint8_t         Signature[8];
    Chunk           chk;
} Header;

typedef struct tagPngPalette
{
    uint8_t         Red;
    uint8_t         Green;
    uint8_t         Blue;
} Palette;

typedef struct tagPngIHDR
{
    uint32_t        Width;
    uint32_t        Height;
    uint8_t         BitDepth;
    uint8_t         ColorType;
    uint8_t         CompressionMethod;
    uint8_t         FilterMethod;
    uint8_t         InterlaceMethod;
} IHDR;

typedef struct tagPngPLTE
{
    Chunk           chk;
    Palette         pal[MAX_PALETTE];
    uint32_t        CrcOfPLTE;
} PLTE;

typedef struct tagPngIDAT
{
} IDAT;

typedef struct tagPngIEND
{
    Chunk           chk;
    uint32_t        CrcOfIEND;
} IEND;

typedef struct tagPngFileHeader
{
    Header          hdr;
    IHDR            ihdr;
    uint32_t        CrcOfIHDR;
} FileHeader;

}

namespace ZLIB
{

typedef struct tagZlibBlock
{
    uint8_t         Flag;
    uint16_t        Length;
    uint16_t        NotLength;  // = ~Length
} Block;

const uint32_t WINDOWSIZE = 32768UL;

}

#include <poppack.h>


enum CVRESULT
{
    CVR_OK = 0,
    CVR_E_ALLOCMEM,
    CVR_E_OPENFILE,
    CVR_E_UNSUPPORTED,
    CVR_E_HEADER,
    CVR_E_CORRUPTED,
    CVR_E_CREATEFILE,
    CVR_E_WRITEFILE,
};

#define ErrorMsgList \
    _T("\0") \
    _T("Out of memory\0") \
    _T("Cannot open file\0") \
    _T("Unknown file format\0") \
    _T("Incorrect bitmap header\0") \
    _T("Corrupted bitmap file\0") \
    _T("Cannot create file\0") \
    _T("Cannot write data\0")


#ifdef _MSC_VER
#define ByteSwap32              _byteswap_ulong
#define ByteSwap16              _byteswap_ushort
#else
#define ByteSwap32              __builtin_bswap32
#define ByteSwap16              __builtin_bswap16
#endif

#define StaticByteSwap32(dw)    \
    static_cast<uint32_t>( \
        ((static_cast<uint32_t>(dw) >> 24) & 0xff) | \
        ((static_cast<uint32_t>(dw) >>  8) & 0xff00) | \
        ((static_cast<uint32_t>(dw) <<  8) & 0xff0000) | \
        ((static_cast<uint32_t>(dw) << 24) & 0xff000000))
#define StaticByteSwap16(w)     \
    static_cast<uint16_t>( \
        ((static_cast<uint16_t>(w) >> 8) & 0xff) | \
        ((static_cast<uint16_t>(w) << 8) & 0xff00))


namespace CRC32
{

const uint32_t INIT    = ~0U;

// half-byte algorithm
// http://create.stephan-brumme.com/crc32/
static const uint32_t c_crc32[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c };

inline uint32_t Calc(__in_bcount(cbSize) const void* buffer, size_t cbSize)
{
    const uint8_t* ptr = static_cast<const uint8_t*>(buffer);
    const uint8_t* ptrEnd = ptr + cbSize;
    uint32_t crc = INIT;
    for (; ptr < ptrEnd; ptr++) {
        uint8_t b = *ptr;
        crc = c_crc32[(crc ^  b      ) & 0x0f] ^ (crc >> 4);
        crc = c_crc32[(crc ^ (b >> 4)) & 0x0f] ^ (crc >> 4);
    }
    return ~crc;
}

}


namespace Adler32
{

const uint32_t BASE    = 65521;
const uint32_t INIT    = 1;

inline uint32_t Calc(__in_bcount(cbSize) const void* buffer, size_t cbSize)
{
    const uint8_t* ptr = static_cast<const uint8_t*>(buffer);
    const uint8_t* ptrEnd = ptr + cbSize;
    uint32_t adler = INIT;
    for (; ptr < ptrEnd; ptr++) {
        uint32_t low = adler & 0xffff;
        uint32_t high = adler >> 16;
        low = (low + *ptr) % BASE;
        high = (high + low) % BASE;
        adler = low | (high << 16);
    }
    return adler;
}

}


class CConverterBase
{
protected:
    uint32_t                cx;
    uint32_t                cy;
    uint32_t                cbSrcWidth;
    uint32_t                cbDestWidth;
    uint32_t                cbSrcImage;
    uint32_t                cbDestImage;
    unsigned int            cPalEntries;
    BMP::Palette*           palette;
    uint8_t*                srcImage;
    uint8_t*                destImage;
    uint8_t                 pngBitDepth;
    uint8_t                 pngColorType;
    BMP::InfoHeader         bmiHeader;

    CConverterBase()
    {
        _STATIC_ASSERT((sizeof(*this) % 4) == 0);
        InlineZeroMemoryDword(this, sizeof(*this));
    }

    ~CConverterBase()
    {
        MemFree(this->palette);
        MemFree(this->srcImage);
    }

    uint32_t Width() const { return this->cx; }
    uint32_t Height() const { return this->cy; }
    uint32_t BmpWidthBytes() const { return this->cbSrcWidth; }
    uint32_t PngWidthBytes() const { return this->cbDestWidth; }
    uint32_t BmpSize() const { return this->cbSrcImage; }
    uint32_t PngSize() const { return this->cbDestImage; }
    const uint8_t* BmpData() const { return this->srcImage; }
    uint8_t* PngData() const { return this->destImage; }
    const BMP::InfoHeader& BitmapHeader() const { return this->bmiHeader; }
};


class CConverter : public CConverterBase
{
private:
    struct CONVERTPROCMAP
    {
        uint8_t bmpBitCount;
        uint8_t bmpCompression;
        uint8_t pngBitDepth;
        uint8_t pngColorType;
        CVRESULT (CConverter::*Proc)();
    };

public:
    INLINE CVRESULT ReadBitmap(CFile& src)
    {
        CVRESULT ret = ParseBitmapHeader(src);
        if (ret == CVR_OK) {
            ret = ParseBitmapPaletteEntries(src);
        }
        if (ret == CVR_OK) {
            ret = src.Read(this->srcImage, this->cbSrcImage) ? CVR_OK : CVR_E_CORRUPTED;
        }
        return ret;
    }

    INLINE CVRESULT WritePng(CFile& dest)
    {
        CVRESULT cvr = WriteHeader(dest);
        if (cvr == CVR_OK) {
            cvr = WritePalette(dest);
        }
        if (cvr == CVR_OK) {
            cvr = WriteData(dest);
        }
        if (cvr == CVR_OK) {
            cvr = WriteEnd(dest);
        }
        return cvr;
    }

    INLINE CVRESULT DoConvert()
    {
        static const CONVERTPROCMAP c_map[] =
        {
            {  1, BMP::RGB,         1, PNG::TYPE_PALETTE,   &CConverter::ConvertBMPItoPNG  },
            {  4, BMP::RGB,         4, PNG::TYPE_PALETTE,   &CConverter::ConvertBMPItoPNG  },
#ifdef SUPPORT_RLE
            {  4, BMP::RLE4,        4, PNG::TYPE_PALETTE,   &CConverter::ConvertRLE4toPNG  },
#endif
            {  8, BMP::RGB,         8, PNG::TYPE_PALETTE,   &CConverter::ConvertBMPItoPNG  },
#ifdef SUPPORT_RLE
            {  8, BMP::RLE8,        8, PNG::TYPE_PALETTE,   &CConverter::ConvertRLE8toPNG  },
#endif
//          { 15, BMP::RGB,         8, PNG::TYPE_RGB,       &CConverter::ConvertBMP15toPNG },
//#ifdef SUPPORT_BITFIELDS
//          { 15, BMP::BITFIELDS    8, PNG::TYPE_RGB,       &CConverter::ConvertBF15toPNG  },
//#endif
//          { 16, BMP::RGB,         8, PNG::TYPE_RGB,       &CConverter::ConvertBMP16toPNG },
//#ifdef SUPPORT_BITFIELDS
//          { 16, BMP::BITFIELDS,   8, PNG::TYPE_RGB,       &CConverter::ConvertBF16toPNG  },
//#endif
            { 24, BMP::RGB,         8, PNG::TYPE_RGB,       &CConverter::ConvertBMP24toPNG },
            { 32, BMP::RGB,         8, PNG::TYPE_RGB_ALPHA, &CConverter::ConvertBMP32toPNG },
//#ifdef SUPPORT_BITFIELDS
//          { 32, BMP::BITFIELDS,   8, PNG::TYPE_RGB_ALPHA, &CConverter::ConvertBF32toPNG  },
//#endif
        };
        const BMP::InfoHeader& bih  = BitmapHeader();
        const uint8_t bitCount      = static_cast<uint8_t>(bih.BitCount);
        const uint8_t compression   = static_cast<uint8_t>(bih.Compression);
        for (rsize_t n = _countof(c_map); n--; ) {
            const CONVERTPROCMAP* const p = c_map + n;
            if (bitCount == p->bmpBitCount && compression == p->bmpCompression) {
                this->pngBitDepth   = p->pngBitDepth;
                this->pngColorType  = p->pngColorType;
                return (this->*p->Proc)();
            }
        }
        return CVR_E_UNSUPPORTED;
    }

private:
    INLINE static bool ValidateBitmapHeader(__in const BMP::FileHeader& bfh, __in const BMP::InfoHeader& bmi)
    {
        if (bfh.Type != StaticByteSwap16('BM')) {
            return false;
        }
        if (static_cast<int32_t>(bfh.OffBits) < 0) {
            return false;
        }
        if (bmi.Size < BMP::SIZE_BITMAPINFO) {
            // TODO: support BITMAPCOREHEADER
//          if (bmi.Size != BMP::SIZE_BITMAPCORE) {
            return false;
//          }
        }
        if (bmi.Width <= 0) {
            return false;
        }
        // TODO: support top-down bitmaps
        if (bmi.Height <= 0) {
            return false;
        }
        // TODO: support planer bitmaps
        if (bmi.Planes != 1) {
            return false;
        }
        if (bmi.BitCount > 32) {
            return false;
        }
        if (bmi.Compression > BMP::BITFIELDS) {
            return false;
        }
        return true;
    }

    INLINE CVRESULT ParseBitmapHeader(CFile& src)
    {
        BMP::FileHeader bfh;
        if (src.Read(&bfh) == false) {
            return CVR_E_UNSUPPORTED;
        }
        if (src.Read(&this->bmiHeader) == false) {
            return CVR_E_UNSUPPORTED;
        }

        if (ValidateBitmapHeader(bfh, this->bmiHeader) == false) {
            return CVR_E_UNSUPPORTED;
        }

        const BMP::InfoHeader& bih = this->bmiHeader;
        const int32_t cx    = bih.Width;
        const int32_t cy    = bih.Height;
        const uint16_t bpp  = bih.BitCount;

        this->cx            = cx;
        this->cy            = cy;
        this->cbSrcWidth    = DIBWIDTHBYTES(this->cx, bpp);
        this->cbDestWidth   = PNGWIDTHBYTES(this->cx, bpp) + 1; // 1 = sizeof(filter)
        this->cbSrcImage    = this->cbSrcWidth * this->cy;
        this->cbDestImage   = this->cbDestWidth * this->cy;

#ifdef SUPPORT_RLE
        if (bih.Compression == BMP::RLE4 || bih.Compression == BMP::RLE8) {
            if (bih.SizeImage == 0) {
                return CVR_E_HEADER;
            }
            this->cbSrcImage = bih.SizeImage;
        }
#endif

        this->srcImage = MemAlloc<uint8_t>(this->cbSrcImage + this->cbDestImage);
        if (this->srcImage != NULL) {
            this->destImage = this->srcImage + this->cbSrcImage;
            return CVR_OK;
        } else {
            return CVR_E_ALLOCMEM;
        }
    }

    INLINE CVRESULT ParseBitmapPaletteEntries(CFile& src)
    {
        const BMP::InfoHeader& bih = BitmapHeader();
        uint32_t cPalEntries = 0;
#ifdef SUPPORT_BITFIELDS
        switch (bih.BitCount) {
        case 1:
        case 4:
        case 8:
            cPalEntries = 1 << bih.BitCount;
            if (bih.ClrUsed > 0) {
                if (bih.ClrUsed > cPalEntries) {
                    return CVR_E_HEADER;
                }
                cPalEntries = bih.ClrUsed;
            }
            break;
        case 24:
            break;
        case 16:
        case 32:
            if (bih.Compression == BMP::BITFIELDS) {
                cPalEntries = 3;
            }
            break;
        default:
            return CVR_E_UNSUPPORTED;
        }
#else
        if (bih.BitCount <= 8) {
            cPalEntries = 1 << bih.BitCount;
            if (bih.ClrUsed > 0) {
                if (bih.ClrUsed > cPalEntries) {
                    return CVR_E_HEADER;
                }
                cPalEntries = bih.ClrUsed;
            }
        }
#endif

        src.Seek(bih.Size - BMP::SIZE_BITMAPINFO, SEEK_CUR);
        if (cPalEntries > 0) {
            if ((this->palette = MemAlloc<BMP::Palette>(cPalEntries)) == NULL) {
                return CVR_E_ALLOCMEM;
            }
//          _ASSERT(src.Seek(0, SEEK_CUR) == sizeof(BITMAPFILE_INFOHEADER));
            if (src.Read(this->palette, cPalEntries * sizeof(BMP::Palette)) == false) {
                return CVR_E_CORRUPTED;
            }
            this->cPalEntries = cPalEntries;
        }
        return CVR_OK;
    }

    NOINLINE static uint32_t CalculateCRC32(const PNG::Chunk* chunk)
    {
        const uint32_t cbSize = sizeof(chunk->type) + ByteSwap32(chunk->cbSize);
        return ByteSwap32(CRC32::Calc(&chunk->type, cbSize));
    }

    INLINE CVRESULT WriteHeader(CFile& dest) const
    {
        _STATIC_ASSERT(sizeof(PNG::FileHeader) == 33);
        static const PNG::Header c_hdr = { PNG_SignatureBytes, StaticByteSwap32(13), PNG::cktype::IHDR };
        PNG::FileHeader png;
        memcpy(&png.hdr, &c_hdr, sizeof(png.hdr));
        png.ihdr.Width              = ByteSwap32(Width());
        png.ihdr.Height             = ByteSwap32(Height());
        png.ihdr.BitDepth           = this->pngBitDepth;
        png.ihdr.ColorType          = this->pngColorType;
        png.ihdr.CompressionMethod  = 0;
        png.ihdr.FilterMethod       = 0;
        png.ihdr.InterlaceMethod    = 0;
        png.CrcOfIHDR               = CalculateCRC32(&png.hdr.chk);
        return dest.Write(&png) ? CVR_OK : CVR_E_WRITEFILE;
    }

    INLINE CVRESULT WritePalette(CFile& dest) const
    {
        if (BitmapHeader().BitCount > 8) {
            return CVR_OK;
        }

        _ASSERT(this->palette != NULL);
        _ASSERT(this->cPalEntries > MAX_PALETTE);

        const unsigned int cPals = this->cPalEntries;
        const uint32_t cbPalSize = cPals * sizeof(PNG::Palette);
        PNG::PLTE plte;
        plte.chk.cbSize     = ByteSwap32(cbPalSize);
        plte.chk.type       = PNG::cktype::PLTE;

        const BMP::Palette* palSrc = this->palette;
        PNG::Palette* palDst = plte.pal;
        for (unsigned int n = cPals; n-- > 0; palSrc++, palDst++) {
            palDst->Red     = palSrc->Red;
            palDst->Green   = palSrc->Green;
            palDst->Blue    = palSrc->Blue;
        }

        uint32_t* const p = reinterpret_cast<uint32_t*>(palDst);
        *p = CalculateCRC32(&plte.chk);

        const uint32_t cb = sizeof(PNG::Chunk) + cbPalSize + sizeof(uint32_t);
        return dest.Write(&plte, cb) ? CVR_OK : CVR_E_WRITEFILE;
    }

    INLINE CVRESULT WriteData(CFile& dest) const
    {
        const uint32_t cbStream = this->cbDestImage + ((this->cbDestImage / 32768) + 1) * sizeof(ZLIB::Block);
        const uint32_t cbIDAT = sizeof(uint16_t)            // zlib header
                                    + cbStream
                                    + sizeof(uint32_t);     // adler32
        const uint32_t cbAllocate = sizeof(PNG::Chunk)      // [cbIDAT]["IDAT"]
                                    + cbIDAT                // IDAT data
                                    + sizeof(uint32_t);     // crc32

        uint8_t* const buffer = MemAlloc<uint8_t>(cbAllocate);
        if (buffer == NULL) {
            return CVR_E_ALLOCMEM;
        }

        PNG::Chunk* chunk = reinterpret_cast<PNG::Chunk*>(buffer);
        chunk->cbSize   = ByteSwap32(cbIDAT);
        chunk->type     = PNG::cktype::IDAT;

        uint16_t* zlibHeader = reinterpret_cast<uint16_t*>(chunk + 1);
        *zlibHeader = StaticByteSwap16(0x78DA);

        uint8_t* const zlibStream = reinterpret_cast<uint8_t*>(zlibHeader + 1);
        uint8_t* ptrDst = zlibStream;
        const uint8_t* ptrSrc = PngData();
        uint32_t cbRemaining = PngSize();

        while (cbRemaining > 0) {
            uint16_t cb = static_cast<uint16_t>(__min(cbRemaining, ZLIB::WINDOWSIZE));
            ZLIB::Block* zblk = reinterpret_cast<ZLIB::Block*>(ptrDst);
            zblk->Flag          = MAKEZLIBFLAG((cbRemaining <= ZLIB::WINDOWSIZE), 0);
            zblk->Length        = cb;
            zblk->NotLength     = ~cb;
            ptrDst += sizeof(ZLIB::Block);
            cbRemaining -= cb;
#if 1
            for (; cb--; *ptrDst++ = *ptrSrc++);
#else
            CopyMemory(ptrDst, ptrSrc, cb);
            ptrDst += cb;
            ptrSrc += cb;
#endif
        }

        uint32_t UNALIGNED* checksums = reinterpret_cast<uint32_t UNALIGNED*>(ptrDst);
        checksums[0] = ByteSwap32(Adler32::Calc(PngData(), PngSize()));
        checksums[1] = CalculateCRC32(chunk);
        CVRESULT ret = dest.Write(buffer, cbAllocate) ? CVR_OK : CVR_E_WRITEFILE;
        MemFree(buffer);
        return ret;
    }

    INLINE CVRESULT WriteEnd(CFile& dest) const
    {
        static const PNG::IEND c_end = { { 0, PNG::cktype::IEND }, StaticByteSwap32(0xAE426082) };
        return dest.Write(&c_end) ? CVR_OK : CVR_E_WRITEFILE;
    }

#ifdef SUPPORT_RLE
    uint32_t CalcSeekPosition(uint32_t x, uint32_t y) const
    {
        uint32_t absPos = y * PngWidthBytes() + x;
        absPos++;       // filter: none
        return absPos;
    }

#ifdef _DEBUG
    inline void ReportBmpOverflow()
    {
        trace("RLE: Buffer overflow\n");
        _CrtDbgBreak();
    }
    inline void ReportPngOverflow()
    {
        trace("PNG: Buffer overflow\n");
        _CrtDbgBreak();
    }
#else
    inline void ReportBmpOverflow() {}
    inline void ReportPngOverflow() {}
#endif

    uint32_t CConverter::DecodeRLE8(
        const uint32_t x,
        const uint32_t y,
        uint8_t b0,
        uint8_t b1,
        __in const CByteStream& srcStream,
        __inout CByteStream& dstStream)
    {
        dstStream.SeekTo(CalcSeekPosition(x, y));
        uint32_t xx = x;
        if (b0 == 0) {
            for (uint8_t i = 0; i < b1; i++, xx++) {
                uint8_t b;
                if (srcStream.Read(&b) == false) {
                    ReportBmpOverflow();
                    return 0;
                }
                if (dstStream.Write(b) == false) {
                    ReportPngOverflow();
                    return 0;
                }
            }
            if (b1 & 1) {
                srcStream.Seek(1);
            }
        } else {
            for (uint8_t i = 0; i < b0; i++, xx++) {
                if (dstStream.Write(b1) == false) {
                    ReportPngOverflow();
                    return 0;
                }
            }
        }
        return xx - x;
    }

    uint32_t CConverter::DecodeRLE4(
        const uint32_t x,
        const uint32_t y,
        uint8_t b0,
        uint8_t b1,
        __in const CByteStream& srcStream,
        __inout CByteStream& dstStream)
    {
        dstStream.SeekTo(CalcSeekPosition(x >> 1, y));
        uint32_t xx = x;
        if (b0 == 0) {
            if (xx & 1) {
                uint8_t d;
                if (dstStream.Peek(&d) == false) {
                    ReportPngOverflow();
                    return 0;
                }
                const uint8_t c = (b1 + 1) / 2;
                for (uint8_t i = 0; i < c; i++, xx += 2) {
                    uint8_t s;
                    if (srcStream.Read(&s) == false) {
                        ReportBmpOverflow();
                        return 0;
                    }
                    uint8_t b = d | ((s >> 4) & 0x0F);
                    if (dstStream.Write(b) == false) {
                        ReportPngOverflow();
                        return 0;
                    }
                    d = ((s << 4) & 0xF0);
                }
                if ((b1 & 1) == 0) {
                    if (dstStream.Write(d) == false) {
                        ReportPngOverflow();
                        return 0;
                    }
                } else {
                    xx--;
                }
            } else {
                const uint8_t c = b1 / 2;
                for (uint8_t i = 0; i < c; i++, xx += 2) {
                    uint8_t b;
                    if (srcStream.Read(&b) == false) {
                        ReportBmpOverflow();
                        return 0;
                    }
                    if (dstStream.Write(b) == false) {
                        ReportPngOverflow();
                        return 0;
                    }
                }
                if (b1 & 1) {
                    uint8_t s, d;
                    if (srcStream.Read(&s) == false) {
                        ReportBmpOverflow();
                        return 0;
                    }
                    d = (s & 0xF0);
                    if (dstStream.Write(d) == false) {
                        ReportPngOverflow();
                        return 0;
                    }
                    xx++;
                }
            }
            if (srcStream.Tell() & 1) {
                srcStream.Seek(1);
            }
        } else {
            if (xx & 1) {
                uint8_t d;
                if (dstStream.Peek(&d) == false) {
                    ReportPngOverflow();
                    return 0;
                }
                d |= (b1 >> 4) & 0x0F;
                dstStream.Write(d);
                b1 = ((b1 >> 4) & 0x0F) | ((b1 << 4) & 0xF0);
                xx++;
                b0--;
            }
            const uint8_t c = b0 / 2;
            for (uint8_t i = 0; i < c; i++, xx += 2) {
                if (dstStream.Write(b1) == false) {
                    ReportPngOverflow();
                    return 0;
                }
            }
            if (b0 & 1) {
                uint8_t b = (b1 & 0xF0);
                if (dstStream.Write(b) == false) {
                    ReportPngOverflow();
                    return 0;
                }
                xx++;
            }
        }
        return xx - x;
    }

    NOINLINE CVRESULT DecodeRLE(uint32_t (CConverter::*func)(const uint32_t x, const uint32_t y, uint8_t b0, uint8_t b1, __in const CByteStream& srcStream, __inout CByteStream& dstStream))
    {
        const uint32_t cy = Height();
        const CByteStream srcStream(BmpData(), BmpSize());
        CByteStream dstStream(PngData(), PngSize());

        uint32_t x = 0, y = cy - 1;

        for (;;) {
            uint8_t b0, b1;
            if (srcStream.Read(&b0) == false || srcStream.Read(&b1) == false) {
                ReportBmpOverflow();
                return CVR_E_CORRUPTED;
            }

            if (b0 == 0) {      // escape
                switch (b1) {
                case 0:         // end of line
                    --y;
                    x = 0;
                    continue;
                case 1:         // end of bitmap
                    goto Exit;
                case 2:         // delta
                    if (srcStream.Read(&b0) == false || srcStream.Read(&b1) == false) {
                        ReportBmpOverflow();
                        return CVR_E_CORRUPTED;
                    }
                    x += b0;
                    y -= b1;
                    continue;
                }
            }

            uint32_t dx = (this->*func)(x, y, b0, b1, srcStream, dstStream);
            if (dx == 0) {
                return CVR_E_CORRUPTED;
            }
            x += dx;
        }

Exit:
        return CVR_OK;
    }
#endif

    NOINLINE CVRESULT ConvertScanline(void (CConverter::*func)(__in const uint8_t* src, __out uint8_t* dst))
    {
        const uint32_t cy               = Height();
        const uint32_t cbSrcWidth       = BmpWidthBytes();
        const uint32_t cbDestWidth      = PngWidthBytes();
        const uint8_t* const srcImage   = BmpData();
        uint8_t* const dstImage         = PngData();

        for (uint32_t y = 0; y < cy; y++) {
            const uint8_t* src = srcImage + (cy - y - 1) * cbSrcWidth;
            uint8_t* dst = dstImage + y * cbDestWidth;
            dst++;  // filter: none
            (this->*func)(src, dst);
        }
        return CVR_OK;
    }

    void ScanI(
        __in const uint8_t* src,
        __out uint8_t* dst)
    {
        const uint32_t cbCopyBytes = PngWidthBytes() - 1;
        InlineCopyMemory(dst, src, cbCopyBytes);
    }

    void Scan24(
        __in const uint8_t* src,
        __out uint8_t* dst)
    {
        const uint32_t cx = Width();
        for (uint32_t x = 0; x < cx; x++) {
            *dst++ = src[2];
            *dst++ = src[1];
            *dst++ = src[0];
            src += 3;
        }
    }

    void Scan32(
        __in const uint8_t* src,
        __out uint8_t* dst)
    {
        const uint32_t cx = Width();
        for (uint32_t x = 0; x < cx; x++) {
            *dst++ = src[2];
            *dst++ = src[1];
            *dst++ = src[0];
            *dst++ = src[3];
            src += 4;
        }
    }

    CVRESULT ConvertBMPItoPNG()
    {
        return ConvertScanline(&CConverter::ScanI);
    }

#ifdef SUPPORT_RLE
    CVRESULT ConvertRLE4toPNG()
    {
        return DecodeRLE(&CConverter::DecodeRLE4);
    }

    CVRESULT ConvertRLE8toPNG()
    {
        return DecodeRLE(&CConverter::DecodeRLE8);
    }
#endif

//  CVRESULT ConvertBMP15toPNG();
//  CVRESULT ConvertBMP16toPNG();

    CVRESULT ConvertBMP24toPNG()
    {
        return ConvertScanline(&CConverter::Scan24);
    }

    CVRESULT ConvertBMP32toPNG()
    {
        return ConvertScanline(&CConverter::Scan32);
    }

#ifdef SUPPORT_BITFIELDS
//  CVRESULT ConvertBF15toPNG ();
//  CVRESULT ConvertBF16toPNG ();
//  CVRESULT ConvertBF32toPNG ();
#endif
};


_TCHAR* MyPathFindExtension(__in const _TCHAR* path)
{
    const _TCHAR* found = NULL;
    while (*path != _T('\0')) {
        switch (*path) {
        case _T('.'):
            found = path;
            break;
        case _T('/'):
        case _T(' '):
#ifdef _WIN32
        case _T('\\'):
#endif
            found = NULL;
            break;
        }
        path = CharNext(path);
    }
    return const_cast<_TCHAR*>(found ? found : path);
}

template <size_t cchExt>
INLINE bool CreateUniqueFile(__inout CFile& file, __in const _TCHAR* infile, __in const _TCHAR (&outExt)[cchExt])
{
    _TCHAR filename[_MAX_PATH];
    const size_t cchIn = InlineStrLen(infile);
    if (cchIn >= _countof(filename)) {
        return false;
    }
    InlineStrCpy(filename, infile);
    const ptrdiff_t posExt = MyPathFindExtension(filename) - filename;
    if (posExt + cchExt + 4 >= _MAX_PATH) {
        return false;
    }
    _TCHAR* ext = filename + posExt;
    memcpy(ext, outExt, cchExt * sizeof(_TCHAR));
    int n = 0;
    while (file.CreateNew(filename) == false) {
        if (n >= 100) {
            return false;
        }
        wsprintf(ext, _T("[%d]%s"), ++n, outExt);
    }
    return true;
}

CVRESULT ProcessFile(__in const _TCHAR* filename)
{
    CConverter conv;
    CFile src, dest;

    CVRESULT ret = src.Open(filename) ? CVR_OK : CVR_E_OPENFILE;
    if (ret == CVR_OK) {
        ret = conv.ReadBitmap(src);
    }
    if (ret == CVR_OK) {
        ret = conv.DoConvert();
    }
    if (ret == CVR_OK) {
        ret = CreateUniqueFile(dest, filename, _T(".png")) ? CVR_OK : CVR_E_CREATEFILE;
    }
    if (ret == CVR_OK) {
        ret = conv.WritePng(dest);
    }
    return ret;
}


namespace App
{

const _TCHAR Title[]    = _T("bmp2png");
const _TCHAR Usage[]    = _T("bmp2png.exe [files ...]");
const _TCHAR MsgAllOK[] = _T("All good !!");

#ifdef TARGET_WIN32GUI
const unsigned int Exclamation  = MB_ICONEXCLAMATION;
const unsigned int Asterisk     = MB_ICONASTERISK;
#else
const unsigned int Exclamation  = 0x30;
const unsigned int Asterisk     = 0x40;
#endif

inline int ShowMessage(__in const _TCHAR* message, unsigned int type)
{
#ifdef MessageBox
    return MessageBox(NULL, message, Title, type);
#else
    fputs(message, type & Exclamation ? stderr : stdout);
    return 0;
#endif
}

INLINE int ShowMessage(__in CVRESULT result, unsigned int type, __in const _TCHAR* filename)
{
    const _TCHAR* message = ErrorMsgList;
    for (unsigned int n = static_cast<unsigned int>(result); n--; ) {
        message += InlineStrLen(message) + 1;
    }
    //const _TCHAR* message = c_ErrorMsgs[result];
#ifdef _UNICODE
    const _TCHAR* format = "%s:\n%S";
#else
    const _TCHAR* format = "%s:\n%s";
#endif
    _TCHAR buffer[1025];
    wsprintf(buffer, format, message, filename);
    return ShowMessage(buffer, type);
}

}


INLINE int Main(int argc, __in_ecount(argc) _TCHAR** argv)
{
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_DELAY_FREE_MEM_DF);
    argc--;
    argv++;

    if (argc <= 0) {
        App::ShowMessage(App::Usage, 0);
        return 0;
    }

    bool failed = false;
    for (int i = 0; i < argc; i++) {
        CVRESULT ret = ProcessFile(argv[i]);
        if (ret != CVR_OK) {
            failed = true;
            App::ShowMessage(ret, App::Exclamation, argv[i]);
        }
    }
    if (failed == false) {
        App::ShowMessage(App::MsgAllOK, App::Asterisk);
    }
    return failed;
}


#ifdef TARGET_WIN32GUI

#ifndef _DEBUG
#pragma comment(linker, "/entry:EntryPoint")
LPTSTR* CommandLineToArgvAorW(__in LPCTSTR cmdLine, __out int* pArgc);

void __cdecl EntryPoint(void)
{
    int mainret = 1;
    LPTSTR pszCommandLine = GetCommandLine();
    if (pszCommandLine != NULL) {
        int argc;
        LPTSTR* argv = CommandLineToArgvAorW(pszCommandLine, &argc);
        if (argv != NULL) {
            mainret = Main(argc, argv);
        }
    }
    ExitProcess(mainret);
}
#else
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
    return Main(__argc, __targv);
}
#endif

// the following code is based on LIBCTINY by Matt Pietrek
// the original code can be found in the MSDN Magazine, January 2001

const size_t MAX_CMDLINE_ARGS   = 128;
const size_t CMDLINE_ARGV_SIZE  = (MAX_CMDLINE_ARGS + 1) * sizeof(LPTSTR);
const TCHAR NIL = _T('\0');
const TCHAR SP = _T(' ');
const TCHAR DQ = _T('"');

inline bool IsCharBlankAorW(TCHAR ch)
{
    return static_cast<_TUCHAR>(ch) <= SP;
}

INLINE LPTSTR* CommandLineToArgvAorW(__in LPCTSTR cmdLine, __out int* pArgc)
{
    *pArgc = 0;

    const SIZE_T cbCmdLine  = static_cast<size_t>(InlineStrLen(cmdLine) + 1) * sizeof(TCHAR);
    const SIZE_T cbAllocate = CMDLINE_ARGV_SIZE + cbCmdLine;
    LPTSTR* const argv  = static_cast<LPTSTR*>(LocalAlloc(LPTR, cbAllocate));

    if (argv == NULL) {
        return NULL;
    }

    LPTSTR ptr = reinterpret_cast<LPTSTR>(reinterpret_cast<BYTE*>(argv) + CMDLINE_ARGV_SIZE);
    InlineStrCpy(ptr, cmdLine);

    if (*ptr == DQ) {
        ptr++;

        argv[0] = ptr;

        while (*ptr != NIL && *ptr != DQ) {
            ptr++;
        }
        if (*ptr == NIL) {
            LocalFree(argv);
            return NULL;
        }
        *ptr++ = NIL;
    } else {
        argv[0] = ptr;
        while (IsCharBlankAorW(*ptr) == false) {
            ptr++;
        }
        if (*ptr != NIL) {
            *ptr++ = NIL;
        }
    }

    int argc = 1;
//  argv[1] = NULL;

    do {
        while (*ptr != NIL && IsCharBlankAorW(*ptr)) {
            ptr++;
        }
        if (*ptr == NIL) {
            break;
        }
        if (*ptr == DQ) {
            ptr++;
            argv[argc++] = ptr;
//          argv[argc] = NULL;

            while (*ptr != NIL && *ptr != DQ) {
                ptr++;
            }
            if (*ptr == NIL) {
                break;
            }
            *ptr++ = NIL;
        } else {
            argv[argc++] = ptr;
//          argv[argc] = NULL;

            while (IsCharBlankAorW(*ptr) == false) {
                ptr++;
            }
            if (*ptr == NIL) {
                break;
            }
            *ptr++ = NIL;
        }
    } while (argc < MAX_CMDLINE_ARGS);

    *pArgc = argc;
    return argv;
}

#else

int main(int argc, char** argv)
{
    return Main(argc, argv);
}

#endif


namespace Debug
{

void Trace(__in __format_string const char* format, ...)
{
    char buffer[1025];
    va_list argPtr;
    va_start(argPtr, format);
#ifdef TARGET_WIN32GUI
    wvsprintf(buffer, format, argPtr);
    OutputDebugString(buffer);
#else
    vsprintf(buffer, format, argPtr);
    printf("DBG: %s\n", buffer);
#endif
    va_end(argPtr);
}

}


#endif /*
!ENDIF
#*/
