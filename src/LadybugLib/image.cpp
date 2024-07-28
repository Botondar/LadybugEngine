#include "image.hpp"

lbfn image_file_type 
DetermineImageFileType(buffer FileData)
{
    image_file_type Result = ImageFile_Undefined;

    for (u32 Candidate = 1;
        (Result == ImageFile_Undefined) && (Candidate < ImageFile_Count);
        Candidate++)
    {
        switch (Candidate)
        {
            case ImageFile_BMP:
            {
                u8 RefTag[2] = { 0x42u, 0x4Du };
                if (FileData.Size >= sizeof(RefTag))
                {
                    if (memcmp(FileData.Data, RefTag, sizeof(RefTag)) == 0)
                    {
                        Result = (image_file_type)Candidate;
                    }
                }
            } break;
            case ImageFile_TIF:
            {
                if (FileData.Size >= 8)
                {
                    u16 ByteOrder = *(u16*)OffsetPtr(FileData.Data, 0);
                    if (ByteOrder == 0x4949u || ByteOrder == 0x4D4Du)
                    {
                        u16 Tag = *(u16*)OffsetPtr(FileData.Data, 2);
                        if (ByteOrder == 0x4D4Du)
                        {
                            Tag = (Tag << 8) | (Tag >> 8);
                        }
                        if (Tag == 42)
                        {
                            Result = (image_file_type)Candidate;
                        }
                    }
                }
            } break;
            case ImageFile_PNG:
            {
                u8 RefTag[8] = { 137u, 80u, 78u, 71u, 13u, 10u, 26u, 10u };
                if (FileData.Size >= sizeof(RefTag))
                {
                    if (memcmp(FileData.Data, RefTag, sizeof(RefTag)) == 0)
                    {
                        Result = (image_file_type)Candidate;
                    }
                }
            } break;
            case ImageFile_JPG:
            {
                // TODO(boti): This tag consists of the JFIF begin marker and JPEG Start-Of-Image bytes, 
                // read the spec more carefully to figure out if we're doing the correct thing here
                u8 RefTag[2] = { 0xFFu, 0xD8u };
                if (FileData.Size >= sizeof(RefTag))
                {
                    if (memcmp(FileData.Data, RefTag, sizeof(RefTag)) == 0)
                    {
                        Result = (image_file_type)Candidate;
                    }
                }
            } break;
            case ImageFile_DDS:
            {
                if (FileData.Size >= DDSHeaderSize)
                {
                    dds_header* Header = (dds_header*)FileData.Data;
                    // TODO(boti): We probably could (and should) check more fields here to ensure that the file is actually .dds
                    dds_flags RequiredFlags = DDSFlag_Caps|DDSFlag_Height|DDSFlag_Width|DDSFlag_PixelFormat;
                    if ((Header->HeaderSize == DDSHeaderSize) && ((Header->Flags & RequiredFlags) == RequiredFlags))
                    {
                        Result = (image_file_type)Candidate;
                    }
                }
            } break;
        }
    }
    return(Result);
}

lbfn f32 CalculateAlphaCoverage(v2u Extent, u8* Texels, f32 AlphaThreshold, f32 AlphaScale)
{
    f32 Coverage = 0.0f;

    u32 Count = Extent.X * Extent.Y;
    f32 Norm = 1.0f / (f32)Count;

    u8* At = Texels + 3;
    while (Count--)
    {
        f32 Alpha = Clamp(AlphaScale * (f32)(*At) / 255.0f, 0.0f, 1.0f);
        At += 4;

        if (Alpha >= AlphaThreshold)
        {
            Coverage += 1.0f;
        }
    }

    Coverage *= Norm;
    return(Coverage);
}

lbfn f32 CalculateAlphaScaleForCoverage(v2u Extent, u8* Texels, f32 AlphaThreshold, f32 TargetCoverage)
{
    f32 MinScale = 0.0f;
    f32 MaxScale = 4.0f;
    f32 Scale = 1.0f;

    for (u32 Step = 0; Step < 8; Step++)
    {
        f32 CurrentCoverage = CalculateAlphaCoverage(Extent, Texels, AlphaThreshold, Scale);

        if      (CurrentCoverage < TargetCoverage) MinScale = Scale;
        else if (CurrentCoverage > TargetCoverage) MaxScale = Scale;
        else    break;

        Scale = 0.5f * (MinScale + MaxScale);
    }

    return(Scale);
}

lbfn f32 RescaleAlphaForCoverage(v2u Extent, u8* Texels, f32 AlphaThreshold, f32 TargetCoverage)
{
    f32 Scale = CalculateAlphaScaleForCoverage(Extent, Texels, AlphaThreshold, TargetCoverage);

    u32 Count = Extent.X * Extent.Y;
    u8* At = Texels + 3;
    while (Count--)
    {
        f32 Alpha = (f32)(*At) / 255.0f;
        Alpha = Clamp(Scale * Alpha, 0.0f, 1.0f);
        u8 Alpha8 = (u8)Round(255.0f * Alpha);
        *At = Alpha8;
        At += 4;
    }

    return(Scale);
}