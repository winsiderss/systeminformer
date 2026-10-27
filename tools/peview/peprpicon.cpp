/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

#include <peview.h>

#define GDIPVER 0x0110
#include <unknwn.h>
#include <gdiplus.h>

using namespace Gdiplus;

static BOOLEAN PvpInitializeGdiPlus(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOLEAN initialized = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        static ULONG_PTR gdiplusToken = 0;
        static GdiplusStartupInput gdiplusStartupInput = { nullptr, FALSE, TRUE };

        if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) == Ok)
            initialized = TRUE;

        PhEndInitOnce(&initOnce);
    }

    return initialized;
}

static HICON PvpGetSectionIconFallback(
    _In_ LONG Dpi
    )
{
    HICON icon = NULL;

    PhGetStockApplicationIcon(&icon, NULL, Dpi);

    return icon;
}

static VOID PvpDrawRoundedRectangle(
    _Inout_ Graphics* Graphics,
    _Inout_ Pen* Pen,
    _In_ REAL X,
    _In_ REAL Y,
    _In_ REAL Width,
    _In_ REAL Height,
    _In_ REAL Radius
    )
{
    GraphicsPath path;
    REAL diameter = Radius * 2.0f;

    path.AddArc(X, Y, diameter, diameter, 180.0f, 90.0f);
    path.AddArc(X + Width - diameter, Y, diameter, diameter, 270.0f, 90.0f);
    path.AddArc(X + Width - diameter, Y + Height - diameter, diameter, diameter, 0.0f, 90.0f);
    path.AddArc(X, Y + Height - diameter, diameter, diameter, 90.0f, 90.0f);
    path.CloseFigure();

    Graphics->DrawPath(Pen, &path);
}

EXTERN_C HICON PvGetSectionIcon(
    _In_ PV_SECTION_ICON_INDEX Index,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ LONG Dpi
    )
{
    Color mainColor;
    Color accentColor;
    HICON icon = NULL;

    if (!PvpInitializeGdiPlus())
        return PvpGetSectionIconFallback(Dpi);

    Bitmap bitmap(Width, Height, PixelFormat32bppPARGB);

    if (bitmap.GetLastStatus() != Ok)
        return PvpGetSectionIconFallback(Dpi);

    Graphics graphics(&bitmap);

    if (graphics.GetLastStatus() != Ok)
        return PvpGetSectionIconFallback(Dpi);

    // The resolved PE Viewer mode, not the Windows preference: an explicit
    // Light/Dark PvThemeMode has to reach the sidebar glyphs too. (dmex)
    if (PvThemeDarkEnabled())
    {
        mainColor = Color(0xff, 0x8a, 0xb4, 0xf8);
        accentColor = Color(0xff, 0x34, 0xd3, 0x99);
    }
    else
    {
        mainColor = Color(0xff, 0x1a, 0x73, 0xe8);
        accentColor = Color(0xff, 0x05, 0x96, 0x69);
    }

    Pen mainPen(mainColor, 1.0f);
    Pen accentPen(accentColor, 1.0f);
    SolidBrush mainBrush(mainColor);
    SolidBrush accentBrush(accentColor);

    mainPen.SetStartCap(LineCapRound);
    mainPen.SetEndCap(LineCapRound);
    mainPen.SetLineJoin(LineJoinRound);
    accentPen.SetStartCap(LineCapRound);
    accentPen.SetEndCap(LineCapRound);
    accentPen.SetLineJoin(LineJoinRound);

    graphics.Clear(Color(0, 0, 0, 0));
    graphics.SetCompositingMode(CompositingModeSourceOver);
    //graphics.SetCompositingQuality(CompositingQualityHighQuality);
    //graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    //graphics.SetPixelOffsetMode(PixelOffsetModeHalf);
    graphics.ScaleTransform((REAL)Width / 16.0f, (REAL)Height / 16.0f);

    switch (Index)
    {
    case PV_SECTION_ICON_GENERAL:
        PvpDrawRoundedRectangle(&graphics, &mainPen, 3.0f, 2.0f, 10.0f, 12.0f, 1.0f);
        graphics.FillRectangle(&mainBrush, 4.0f, 3.0f, 8.0f, 3.0f);
        graphics.DrawLine(&mainPen, 5.0f, 8.0f, 11.0f, 8.0f);
        graphics.DrawLine(&mainPen, 5.0f, 10.0f, 10.0f, 10.0f);
        break;
    case PV_SECTION_ICON_HEADERS:
        graphics.DrawRectangle(&mainPen, 2.0f, 3.0f, 12.0f, 10.0f);
        graphics.FillRectangle(&mainBrush, 2.0f, 3.0f, 12.0f, 3.0f);
        graphics.DrawLine(&mainPen, 7.0f, 6.0f, 7.0f, 13.0f);
        graphics.DrawLine(&mainPen, 2.0f, 9.0f, 14.0f, 9.0f);
        break;
    case PV_SECTION_ICON_LOAD_CONFIG:
        graphics.DrawEllipse(&mainPen, 3.0f, 3.0f, 10.0f, 10.0f);
        graphics.DrawLine(&mainPen, 8.0f, 1.0f, 8.0f, 15.0f);
        graphics.DrawLine(&mainPen, 1.0f, 8.0f, 15.0f, 8.0f);
        graphics.DrawLine(&mainPen, 3.0f, 3.0f, 13.0f, 13.0f);
        graphics.DrawLine(&mainPen, 3.0f, 13.0f, 13.0f, 3.0f);
        graphics.FillEllipse(&mainBrush, 6.0f, 6.0f, 4.0f, 4.0f);
        break;
    case PV_SECTION_ICON_SECTIONS:
        PvpDrawRoundedRectangle(&graphics, &mainPen, 2.0f, 2.0f, 8.0f, 8.0f, 1.0f);
        PvpDrawRoundedRectangle(&graphics, &accentPen, 6.0f, 6.0f, 8.0f, 8.0f, 1.0f);
        break;
    case PV_SECTION_ICON_DIRECTORIES:
        {
            PointF tabPoints[] = {
                PointF(2.0f, 5.0f), PointF(2.0f, 3.0f),
                PointF(6.0f, 3.0f), PointF(8.0f, 5.0f)
                };

            graphics.DrawLines(&mainPen, tabPoints, RTL_NUMBER_OF(tabPoints));
            PvpDrawRoundedRectangle(&graphics, &mainPen, 2.0f, 5.0f, 12.0f, 8.0f, 1.0f);
        }
        break;
    case PV_SECTION_ICON_IMPORTS:
        {
            PointF trayPoints[] = {
                PointF(3.0f, 7.0f), PointF(3.0f, 13.0f),
                PointF(13.0f, 13.0f), PointF(13.0f, 7.0f)
                };
            PointF arrowPoints[] = {
                PointF(5.0f, 7.0f), PointF(8.0f, 10.0f), PointF(11.0f, 7.0f)
                };

            graphics.DrawLines(&mainPen, trayPoints, RTL_NUMBER_OF(trayPoints));
            graphics.DrawLine(&accentPen, 8.0f, 2.0f, 8.0f, 10.0f);
            graphics.DrawLines(&accentPen, arrowPoints, RTL_NUMBER_OF(arrowPoints));
        }
        break;
    case PV_SECTION_ICON_EXPORTS:
        {
            PointF trayPoints[] = {
                PointF(3.0f, 8.0f), PointF(3.0f, 14.0f),
                PointF(13.0f, 14.0f), PointF(13.0f, 8.0f)
                };
            PointF arrowPoints[] = {
                PointF(5.0f, 7.0f), PointF(8.0f, 4.0f), PointF(11.0f, 7.0f)
                };

            graphics.DrawLines(&mainPen, trayPoints, RTL_NUMBER_OF(trayPoints));
            graphics.DrawLine(&accentPen, 8.0f, 12.0f, 8.0f, 4.0f);
            graphics.DrawLines(&accentPen, arrowPoints, RTL_NUMBER_OF(arrowPoints));
        }
        break;
    case PV_SECTION_ICON_RESOURCES:
        {
            PointF topPoints[] = {
                PointF(8.0f, 2.0f), PointF(13.0f, 5.0f),
                PointF(8.0f, 8.0f), PointF(3.0f, 5.0f)
                };
            PointF leftPoints[] = {
                PointF(3.0f, 5.0f), PointF(3.0f, 11.0f), PointF(8.0f, 14.0f)
                };
            PointF rightPoints[] = {
                PointF(13.0f, 5.0f), PointF(13.0f, 11.0f), PointF(8.0f, 14.0f)
                };

            graphics.DrawPolygon(&mainPen, topPoints, RTL_NUMBER_OF(topPoints));
            graphics.DrawLine(&mainPen, 8.0f, 8.0f, 8.0f, 14.0f);
            graphics.DrawLines(&mainPen, leftPoints, RTL_NUMBER_OF(leftPoints));
            graphics.DrawLines(&mainPen, rightPoints, RTL_NUMBER_OF(rightPoints));
        }
        break;
    case PV_SECTION_ICON_CFG:
        {
            PointF shieldPoints[] = {
                PointF(3.0f, 3.0f), PointF(13.0f, 3.0f), PointF(13.0f, 7.0f),
                PointF(8.0f, 13.0f), PointF(3.0f, 7.0f)
                };

            graphics.DrawPolygon(&mainPen, shieldPoints, RTL_NUMBER_OF(shieldPoints));
            graphics.FillEllipse(&accentBrush, 6.0f, 5.0f, 4.0f, 4.0f);
        }
        break;
    case PV_SECTION_ICON_PDBID:
        PvpDrawRoundedRectangle(&graphics, &mainPen, 2.0f, 3.0f, 12.0f, 10.0f, 1.0f);
        graphics.DrawRectangle(&mainPen, 4.0f, 5.0f, 4.0f, 6.0f);
        graphics.DrawLine(&mainPen, 9.0f, 6.0f, 12.0f, 6.0f);
        graphics.DrawLine(&mainPen, 9.0f, 9.0f, 12.0f, 9.0f);
        break;
    case PV_SECTION_ICON_EXCEPTIONS:
        {
            PointF trianglePoints[] = {
                PointF(8.0f, 2.0f), PointF(14.0f, 13.0f), PointF(2.0f, 13.0f)
                };

            graphics.DrawPolygon(&accentPen, trianglePoints, RTL_NUMBER_OF(trianglePoints));
            graphics.DrawLine(&accentPen, 8.0f, 5.0f, 8.0f, 9.0f);
            graphics.FillEllipse(&accentBrush, 7.25f, 10.25f, 1.5f, 1.5f);
        }
        break;
    case PV_SECTION_ICON_RELOCATIONS:
        {
            PointF firstArrow[] = {
                PointF(10.0f, 2.0f), PointF(13.0f, 5.0f), PointF(10.0f, 7.0f)
                };
            PointF secondArrow[] = {
                PointF(6.0f, 14.0f), PointF(3.0f, 11.0f), PointF(6.0f, 9.0f)
                };

            graphics.DrawArc(&mainPen, 3.0f, 3.0f, 10.0f, 10.0f, 210.0f, 120.0f);
            graphics.DrawArc(&mainPen, 3.0f, 3.0f, 10.0f, 10.0f, 30.0f, 120.0f);
            graphics.DrawLines(&mainPen, firstArrow, RTL_NUMBER_OF(firstArrow));
            graphics.DrawLines(&mainPen, secondArrow, RTL_NUMBER_OF(secondArrow));
        }
        break;
    case PV_SECTION_ICON_CERTIFICATES:
        {
            PointF leftRibbon[] = {
                PointF(5.0f, 9.0f), PointF(4.0f, 14.0f),
                PointF(6.0f, 12.0f), PointF(7.0f, 9.0f)
                };
            PointF rightRibbon[] = {
                PointF(9.0f, 9.0f), PointF(10.0f, 12.0f),
                PointF(12.0f, 14.0f), PointF(11.0f, 9.0f)
                };

            graphics.DrawEllipse(&mainPen, 4.0f, 2.0f, 8.0f, 8.0f);
            graphics.DrawPolygon(&mainPen, leftRibbon, RTL_NUMBER_OF(leftRibbon));
            graphics.DrawPolygon(&mainPen, rightRibbon, RTL_NUMBER_OF(rightRibbon));
        }
        break;
    case PV_SECTION_ICON_DEBUG:
        graphics.DrawEllipse(&mainPen, 5.0f, 5.0f, 6.0f, 7.0f);
        graphics.DrawEllipse(&mainPen, 6.0f, 2.0f, 4.0f, 3.0f);
        graphics.DrawLine(&mainPen, 2.0f, 6.0f, 5.0f, 6.0f);
        graphics.DrawLine(&mainPen, 11.0f, 6.0f, 14.0f, 6.0f);
        graphics.DrawLine(&mainPen, 2.0f, 8.0f, 5.0f, 8.0f);
        graphics.DrawLine(&mainPen, 11.0f, 8.0f, 14.0f, 8.0f);
        graphics.DrawLine(&mainPen, 2.0f, 11.0f, 5.0f, 11.0f);
        graphics.DrawLine(&mainPen, 11.0f, 11.0f, 14.0f, 11.0f);
        break;
    case PV_SECTION_ICON_VOLATILE:
        {
            PointF leftBracket[] = {
                PointF(6.0f, 3.0f), PointF(4.0f, 3.0f), PointF(4.0f, 7.0f),
                PointF(2.0f, 8.0f), PointF(4.0f, 9.0f), PointF(4.0f, 13.0f),
                PointF(6.0f, 13.0f)
                };
            PointF rightBracket[] = {
                PointF(10.0f, 3.0f), PointF(12.0f, 3.0f), PointF(12.0f, 7.0f),
                PointF(14.0f, 8.0f), PointF(12.0f, 9.0f), PointF(12.0f, 13.0f),
                PointF(10.0f, 13.0f)
                };

            graphics.DrawLines(&mainPen, leftBracket, RTL_NUMBER_OF(leftBracket));
            graphics.DrawLines(&mainPen, rightBracket, RTL_NUMBER_OF(rightBracket));
        }
        break;
    case PV_SECTION_ICON_EHCONT:
        {
            PointF leftChevron[] = {
                PointF(6.0f, 4.0f), PointF(3.0f, 8.0f), PointF(6.0f, 12.0f)
                };
            PointF rightChevron[] = {
                PointF(10.0f, 4.0f), PointF(13.0f, 8.0f), PointF(10.0f, 12.0f)
                };

            graphics.DrawLines(&mainPen, leftChevron, RTL_NUMBER_OF(leftChevron));
            graphics.DrawLines(&mainPen, rightChevron, RTL_NUMBER_OF(rightChevron));
        }
        break;
    case PV_SECTION_ICON_POGO:
        graphics.DrawEllipse(&mainPen, 3.0f, 3.0f, 10.0f, 10.0f);
        graphics.FillEllipse(&mainBrush, 7.0f, 7.0f, 2.0f, 2.0f);
        graphics.DrawLine(&mainPen, 8.0f, 1.0f, 8.0f, 15.0f);
        graphics.DrawLine(&mainPen, 1.0f, 8.0f, 15.0f, 8.0f);
        break;
    case PV_SECTION_ICON_CRT:
        {
            PointF promptPoints[] = {
                PointF(4.0f, 7.0f), PointF(6.0f, 8.0f), PointF(4.0f, 10.0f)
                };

            PvpDrawRoundedRectangle(&graphics, &mainPen, 2.0f, 3.0f, 12.0f, 10.0f, 1.0f);
            graphics.DrawLine(&mainPen, 2.0f, 5.0f, 14.0f, 5.0f);
            graphics.DrawLines(&mainPen, promptPoints, RTL_NUMBER_OF(promptPoints));
            graphics.DrawLine(&mainPen, 8.0f, 10.0f, 11.0f, 10.0f);
        }
        break;
    case PV_SECTION_ICON_PROPERTIES:
        graphics.DrawLine(&mainPen, 2.0f, 4.0f, 14.0f, 4.0f);
        graphics.DrawLine(&mainPen, 2.0f, 8.0f, 14.0f, 8.0f);
        graphics.DrawLine(&mainPen, 2.0f, 12.0f, 14.0f, 12.0f);
        graphics.FillEllipse(&mainBrush, 5.0f, 3.0f, 2.0f, 2.0f);
        graphics.FillEllipse(&mainBrush, 9.0f, 7.0f, 2.0f, 2.0f);
        graphics.FillEllipse(&mainBrush, 4.0f, 11.0f, 2.0f, 2.0f);
        break;
    case PV_SECTION_ICON_ATTRIBUTES:
        {
            PointF tagPoints[] = {
                PointF(3.0f, 8.0f), PointF(8.0f, 3.0f), PointF(14.0f, 3.0f),
                PointF(14.0f, 13.0f), PointF(3.0f, 13.0f)
                };

            graphics.DrawPolygon(&mainPen, tagPoints, RTL_NUMBER_OF(tagPoints));
            graphics.DrawEllipse(&mainPen, 10.0f, 5.0f, 2.0f, 2.0f);
        }
        break;
    case PV_SECTION_ICON_STREAMS:
        {
            PointF firstWave[] = {
                PointF(2.0f, 4.0f), PointF(5.0f, 2.0f), PointF(8.0f, 4.0f),
                PointF(11.0f, 6.0f), PointF(14.0f, 4.0f)
                };
            PointF secondWave[] = {
                PointF(2.0f, 8.0f), PointF(5.0f, 6.0f), PointF(8.0f, 8.0f),
                PointF(11.0f, 10.0f), PointF(14.0f, 8.0f)
                };
            PointF thirdWave[] = {
                PointF(2.0f, 12.0f), PointF(5.0f, 10.0f), PointF(8.0f, 12.0f),
                PointF(11.0f, 14.0f), PointF(14.0f, 12.0f)
                };

            graphics.DrawLines(&mainPen, firstWave, RTL_NUMBER_OF(firstWave));
            graphics.DrawLines(&mainPen, secondWave, RTL_NUMBER_OF(secondWave));
            graphics.DrawLines(&mainPen, thirdWave, RTL_NUMBER_OF(thirdWave));
        }
        break;
    case PV_SECTION_ICON_LAYOUT:
        graphics.DrawRectangle(&mainPen, 2.0f, 2.0f, 12.0f, 12.0f);
        graphics.DrawLine(&mainPen, 8.0f, 2.0f, 8.0f, 14.0f);
        graphics.DrawLine(&mainPen, 2.0f, 8.0f, 14.0f, 8.0f);
        graphics.FillRectangle(&mainBrush, 3.0f, 3.0f, 4.0f, 4.0f);
        break;
    case PV_SECTION_ICON_LINKS:
        PvpDrawRoundedRectangle(&graphics, &mainPen, 2.0f, 5.0f, 7.0f, 6.0f, 3.0f);
        PvpDrawRoundedRectangle(&graphics, &mainPen, 7.0f, 5.0f, 7.0f, 6.0f, 3.0f);
        break;
    case PV_SECTION_ICON_PROCESSES:
        graphics.DrawRectangle(&mainPen, 4.0f, 4.0f, 8.0f, 8.0f);
        graphics.FillRectangle(&mainBrush, 7.0f, 7.0f, 2.0f, 2.0f);
        graphics.DrawLine(&mainPen, 6.0f, 1.0f, 6.0f, 4.0f);
        graphics.DrawLine(&mainPen, 10.0f, 1.0f, 10.0f, 4.0f);
        graphics.DrawLine(&mainPen, 6.0f, 12.0f, 6.0f, 15.0f);
        graphics.DrawLine(&mainPen, 10.0f, 12.0f, 10.0f, 15.0f);
        graphics.DrawLine(&mainPen, 1.0f, 6.0f, 4.0f, 6.0f);
        graphics.DrawLine(&mainPen, 1.0f, 10.0f, 4.0f, 10.0f);
        graphics.DrawLine(&mainPen, 12.0f, 6.0f, 15.0f, 6.0f);
        graphics.DrawLine(&mainPen, 12.0f, 10.0f, 15.0f, 10.0f);
        break;
    case PV_SECTION_ICON_HASHES:
        graphics.DrawLine(&mainPen, 6.0f, 2.0f, 5.0f, 14.0f);
        graphics.DrawLine(&mainPen, 11.0f, 2.0f, 10.0f, 14.0f);
        graphics.DrawLine(&mainPen, 2.0f, 6.0f, 14.0f, 6.0f);
        graphics.DrawLine(&mainPen, 2.0f, 10.0f, 14.0f, 10.0f);
        break;
    case PV_SECTION_ICON_PREVIEW:
        {
            PointF topEye[] = {
                PointF(2.0f, 8.0f), PointF(5.0f, 4.0f), PointF(8.0f, 4.0f),
                PointF(11.0f, 4.0f), PointF(14.0f, 8.0f)
                };
            PointF bottomEye[] = {
                PointF(2.0f, 8.0f), PointF(5.0f, 12.0f), PointF(8.0f, 12.0f),
                PointF(11.0f, 12.0f), PointF(14.0f, 8.0f)
                };

            graphics.DrawLines(&mainPen, topEye, RTL_NUMBER_OF(topEye));
            graphics.DrawLines(&mainPen, bottomEye, RTL_NUMBER_OF(bottomEye));
            graphics.FillEllipse(&mainBrush, 6.0f, 6.0f, 4.0f, 4.0f);
        }
        break;
    case PV_SECTION_ICON_SYMBOLS:
        {
            PointF stemPoints[] = {
                PointF(4.0f, 13.0f), PointF(6.0f, 11.0f),
                PointF(6.0f, 5.0f), PointF(8.0f, 3.0f)
                };

            graphics.DrawLines(&mainPen, stemPoints, RTL_NUMBER_OF(stemPoints));
            graphics.DrawLine(&mainPen, 4.0f, 7.0f, 8.0f, 7.0f);
            graphics.DrawLine(&mainPen, 9.0f, 8.0f, 13.0f, 12.0f);
            graphics.DrawLine(&mainPen, 13.0f, 8.0f, 9.0f, 12.0f);
        }
        break;
    case PV_SECTION_ICON_STRINGS:
        {
            PointF firstQuote[] = {
                PointF(4.0f, 5.0f), PointF(6.0f, 5.0f),
                PointF(5.0f, 8.0f), PointF(4.0f, 8.0f)
                };
            PointF secondQuote[] = {
                PointF(7.0f, 5.0f), PointF(9.0f, 5.0f),
                PointF(8.0f, 8.0f), PointF(7.0f, 8.0f)
                };
            PointF thirdQuote[] = {
                PointF(10.0f, 5.0f), PointF(12.0f, 5.0f),
                PointF(11.0f, 8.0f), PointF(10.0f, 8.0f)
                };
            PointF fourthQuote[] = {
                PointF(13.0f, 5.0f), PointF(15.0f, 5.0f),
                PointF(14.0f, 8.0f), PointF(13.0f, 8.0f)
                };

            graphics.FillPolygon(&mainBrush, firstQuote, RTL_NUMBER_OF(firstQuote));
            graphics.FillPolygon(&mainBrush, secondQuote, RTL_NUMBER_OF(secondQuote));
            graphics.FillPolygon(&mainBrush, thirdQuote, RTL_NUMBER_OF(thirdQuote));
            graphics.FillPolygon(&mainBrush, fourthQuote, RTL_NUMBER_OF(fourthQuote));
        }
        break;
    case PV_SECTION_ICON_VERSION:
        {
            PointF tagPoints[] = {
                PointF(4.0f, 2.0f), PointF(12.0f, 2.0f), PointF(12.0f, 14.0f),
                PointF(8.0f, 11.0f), PointF(4.0f, 14.0f)
                };

            graphics.DrawPolygon(&mainPen, tagPoints, RTL_NUMBER_OF(tagPoints));
            graphics.FillEllipse(&mainBrush, 7.0f, 4.0f, 2.0f, 2.0f);
        }
        break;
    case PV_SECTION_ICON_MUI:
        graphics.DrawEllipse(&mainPen, 2.0f, 2.0f, 12.0f, 12.0f);
        graphics.DrawLine(&mainPen, 2.0f, 8.0f, 14.0f, 8.0f);
        graphics.DrawEllipse(&mainPen, 5.0f, 2.0f, 6.0f, 12.0f);
        break;
    case PV_SECTION_ICON_ANOMALIES:
        {
            PointF trianglePoints[] = {
                PointF(8.0f, 2.5f), PointF(14.0f, 13.0f), PointF(2.0f, 13.0f)
                };

            graphics.DrawPolygon(&mainPen, trianglePoints, RTL_NUMBER_OF(trianglePoints));
            graphics.DrawLine(&mainPen, 8.0f, 6.5f, 8.0f, 9.5f);
            graphics.FillEllipse(&mainBrush, 7.4f, 10.6f, 1.2f, 1.2f);
        }
        break;
    case PV_SECTION_ICON_GETLOADLIBRARY:
        PvpDrawRoundedRectangle(&graphics, &mainPen, 3.0f, 2.0f, 10.0f, 3.0f, 0.5f);
        PvpDrawRoundedRectangle(&graphics, &mainPen, 3.0f, 6.0f, 10.0f, 3.0f, 0.5f);
        PvpDrawRoundedRectangle(&graphics, &mainPen, 3.0f, 10.0f, 10.0f, 3.0f, 0.5f);
        break;
    case PV_SECTION_ICON_GETPROCADDR:
        {
            PointF functionLine[] = {
                PointF(3.0f, 11.0f), PointF(5.0f, 9.0f),
                PointF(5.0f, 5.0f), PointF(7.0f, 3.0f)
                };
            PointF arrowPoints[] = {
                PointF(12.0f, 6.0f), PointF(14.0f, 8.0f), PointF(12.0f, 10.0f)
                };

            graphics.DrawLines(&mainPen, functionLine, RTL_NUMBER_OF(functionLine));
            graphics.DrawLine(&mainPen, 8.0f, 8.0f, 14.0f, 8.0f);
            graphics.DrawLines(&mainPen, arrowPoints, RTL_NUMBER_OF(arrowPoints));
        }
        break;
    default:
        PvpDrawRoundedRectangle(&graphics, &mainPen, 3.0f, 2.0f, 10.0f, 12.0f, 1.0f);
        break;
    }

    graphics.ResetTransform();

    if (graphics.GetLastStatus() != Ok || bitmap.GetHICON(&icon) != Ok || !icon)
    {
        if (icon)
            DestroyIcon(icon);

        return PvpGetSectionIconFallback(Dpi);
    }

    return icon;
}

// The glyph beside a metadata category heading. Unlike the section icons these are
// drawn in the accent color the category carries, so the heading, its glyph and its
// collapse affordance read as one unit.
EXTERN_C HICON PvGetCategoryIcon(
    _In_ PV_THEME_ACCENT Accent,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ LONG Dpi
    )
{
    COLORREF accentColor;
    HICON icon = NULL;

    if (!PvpInitializeGdiPlus())
        return nullptr;

    Bitmap bitmap(Width, Height, PixelFormat32bppPARGB);

    if (bitmap.GetLastStatus() != Ok)
        return nullptr;

    Graphics graphics(&bitmap);

    if (graphics.GetLastStatus() != Ok)
        return nullptr;

    accentColor = PvGetThemeAccentColor(Accent);

    Color glyphColor(0xff, GetRValue(accentColor), GetGValue(accentColor), GetBValue(accentColor));
    Pen glyphPen(glyphColor, 1.0f);
    SolidBrush glyphBrush(glyphColor);

    glyphPen.SetStartCap(LineCapRound);
    glyphPen.SetEndCap(LineCapRound);
    glyphPen.SetLineJoin(LineJoinRound);

    graphics.Clear(Color(0, 0, 0, 0));
    graphics.SetCompositingMode(CompositingModeSourceOver);
    graphics.ScaleTransform((REAL)Width / 16.0f, (REAL)Height / 16.0f);

    switch (Accent)
    {
    case PvThemeAccentDebug:
        {
            // A bug: body, head and legs.
            graphics.DrawEllipse(&glyphPen, 5.0f, 5.0f, 6.0f, 8.0f);
            graphics.DrawEllipse(&glyphPen, 6.5f, 2.0f, 3.0f, 3.0f);
            graphics.DrawLine(&glyphPen, 2.0f, 7.0f, 5.0f, 8.0f);
            graphics.DrawLine(&glyphPen, 2.0f, 12.0f, 5.0f, 11.0f);
            graphics.DrawLine(&glyphPen, 14.0f, 7.0f, 11.0f, 8.0f);
            graphics.DrawLine(&glyphPen, 14.0f, 12.0f, 11.0f, 11.0f);
        }
        break;
    case PvThemeAccentTrust:
        {
            // A shield with a check inside it.
            PointF shield[] = {
                PointF(8.0f, 2.0f), PointF(13.0f, 4.0f), PointF(13.0f, 8.0f),
                PointF(8.0f, 14.0f), PointF(3.0f, 8.0f), PointF(3.0f, 4.0f)
                };
            PointF check[] = {
                PointF(6.0f, 8.0f), PointF(7.5f, 9.5f), PointF(10.5f, 6.0f)
                };

            graphics.DrawPolygon(&glyphPen, shield, RTL_NUMBER_OF(shield));
            graphics.DrawLines(&glyphPen, check, RTL_NUMBER_OF(check));
        }
        break;
    case PvThemeAccentInternal:
        {
            // A drive platter, for the on-disk metadata.
            graphics.DrawEllipse(&glyphPen, 2.5f, 3.0f, 11.0f, 4.0f);
            graphics.DrawLine(&glyphPen, 2.5f, 5.0f, 2.5f, 11.0f);
            graphics.DrawLine(&glyphPen, 13.5f, 5.0f, 13.5f, 11.0f);
            graphics.DrawArc(&glyphPen, 2.5f, 9.0f, 11.0f, 4.0f, 0.0f, 180.0f);
        }
        break;
    case PvThemeAccentPrimary:
    case PvThemeAccentInfo:
    default:
        {
            // An info circle.
            graphics.DrawEllipse(&glyphPen, 2.0f, 2.0f, 12.0f, 12.0f);
            graphics.FillRectangle(&glyphBrush, 7.5f, 4.5f, 1.0f, 1.0f);
            graphics.DrawLine(&glyphPen, 8.0f, 7.0f, 8.0f, 11.5f);
        }
        break;
    }

    graphics.ResetTransform();

    if (graphics.GetLastStatus() != Ok || bitmap.GetHICON(&icon) != Ok || !icon)
    {
        if (icon)
            DestroyIcon(icon);

        return nullptr;
    }

    return icon;
}
