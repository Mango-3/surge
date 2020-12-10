#include "CSurgeVuMeter.h"
#include "DspUtilities.h"
#include "SkinColors.h"

using namespace VSTGUI;

CSurgeVuMeter::CSurgeVuMeter(const CRect& size, VSTGUI::IControlListener *listener) : CControl(size, listener, 0, 0)
{
   stereo = true;
   valueR = 0.0;
   setMax( 2.0 ); // since this can clip values are above 1 sometimes
}

float scale(float x)
{
   x = limit_range(0.5f * x, 0.f, 1.f);
   return powf(x, 0.3333333333f);
}

CMouseEventResult CSurgeVuMeter::onMouseDown(CPoint& where, const CButtonState& button)
{
   if (listener && (button & (kMButton | kButton4 | kButton5)))
   {
      listener->controlModifierClicked(this, button);
      return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
   }
   return CControl::onMouseDown(where, button);
}

void CSurgeVuMeter::setType(int vutype)
{
   type = vutype;
   switch (type)
   {
   case vut_vu:
      stereo = false;
      break;
   case vut_vu_stereo:
      stereo = true;
      break;
   case vut_gain_reduction:
      stereo = false;
      break;
   default:
      type = 0;
      break;
   }
}

// void setSecondaryValue(float v);

void CSurgeVuMeter::setValueR(float f)
{
   if (f != valueR)
   {
      valueR = f;
      setDirty();
   }
}

void CSurgeVuMeter::draw(CDrawContext* dc)
{
   auto vugreen = skin->getColor(Colors::VuMeter::Level);
   auto vugreenNotch = skin->getColor(Colors::VuMeter::LevelNotch);

   auto vuamber = skin->getColor(Colors::VuMeter::MidLevel);
   auto vuamberNotch = skin->getColor(Colors::VuMeter::MidLevelNotch);

   auto vured = skin->getColor(Colors::VuMeter::HighLevel);
   auto vuredNotch = skin->getColor(Colors::VuMeter::HighLevelNotch);

   auto vugr = skin->getColor(Colors::VuMeter::GainReductionLevel);
   auto vugrNotch = skin->getColor(Colors::VuMeter::GainReductionLevelNotch);

   auto notchDistance = 2;

   auto borderCol = skin->getColor(Colors::VuMeter::Border);


   CRect componentSize = getViewSize();

   VSTGUI::CDrawMode origMode = dc->getDrawMode();
   VSTGUI::CDrawMode newMode(VSTGUI::kAntiAliasing | VSTGUI::kNonIntegralMode );
   dc->setDrawMode(newMode);

   dc->setFillColor(skin->getColor(Colors::VuMeter::Background));
   dc->drawRect(componentSize, VSTGUI::kDrawFilled);

   CRect borderRoundedRectBox = componentSize;
   borderRoundedRectBox.inset(1, 1);
   VSTGUI::CGraphicsPath* borderRoundedRectPath = dc->createRoundRectGraphicsPath(borderRoundedRectBox, 2);

   dc->setFillColor(borderCol);
   dc->drawGraphicsPath(borderRoundedRectPath, VSTGUI::CDrawContext::kPathFilled);

   CRect vuBarRegion(componentSize);
   vuBarRegion.inset(2, 2);

   float w = vuBarRegion.getWidth();
   /*
    * Where does this constant come from?
    * Well in 'scale' we map x -> x^3 to compress the range
    * cuberoot(2) = 1.26 and change
    * 1 / cuberoot(2) = 0.793
    * so this scales it so the zerodb point is 1/cuberoot(2) along
    * still not entirely sure why but there you go
    */
   int zerodb = (0.7937f * w);
   /*
    * but if we are doing that, make 3 the amber point
    * 1/cuberoot(3) = .69336
    */
   int amberdb = (0.69336f * w);

   if (type == vut_gain_reduction)
   {
      // OK we need to draw from zerodb to the value
      int yMargin = 1;
      auto zeroDbPoint = CPoint( vuBarRegion.left + zerodb, vuBarRegion.top + yMargin );
      auto displayRect = CRect( zeroDbPoint,
                               CPoint( (scale(value)*w) - zerodb, vuBarRegion.getHeight() - yMargin ));

      // The displayRect ends up correctly sized but a bit heavy handed so
      displayRect.top += 0.5;
      displayRect.bottom -= 0.5;

      dc->setFillColor(vugr);
      dc->drawRect( displayRect, kDrawFilled);

      // Draw over the notches
      auto start = displayRect.left;
      auto end = displayRect.right;
      if( displayRect.getWidth() < 0 )
      {
         auto notches = ceil( displayRect.getWidth() / notchDistance );
         start = start + notches * notchDistance;
         end = displayRect.left;
      }

      dc->setFillColor(vugrNotch);
      for( auto pos = start; pos <= end; pos += notchDistance )
      {
         auto notch = CRect(CPoint(pos,displayRect.top),CPoint(1,displayRect.getHeight()));
         dc->drawRect( notch, kDrawFilled );
      }

   }
   else
   {
      auto vuBarLeft = vuBarRegion;
      auto vuBarRight = vuBarRegion;

      vuBarLeft.right = vuBarLeft.left + scale(value) * w;

      if (stereo)
      {
         auto center = vuBarLeft.top + vuBarLeft.getHeight() / 2;
         vuBarLeft.bottom = center;
         vuBarRight.top = center;
         vuBarRight.offset(0,1); // jog down one pixel to make a gap
         vuBarRight.right = vuBarRight.left + scale(valueR) * w;
      }

      auto drawMultiNotchedBar = [&](const CRect &targetBar )
      {
         auto drawThis = targetBar;
         auto zp = vuBarLeft.left + zerodb;
         auto ap = vuBarLeft.left + amberdb;
         if( drawThis.right > zp )
         {
            dc->setFillColor(vured);
            dc->drawRect(drawThis,kDrawFilled);
            dc->setFillColor(vuredNotch);
            for( auto pos = drawThis.left; pos <= drawThis.right; pos += notchDistance )
            {
               auto notch = CRect(CPoint(pos,drawThis.top),CPoint(1,drawThis.getHeight()));
               dc->drawRect( notch, kDrawFilled );
            }
            drawThis.right = zp;

         }
         if( drawThis.right >= zp )
         {
            dc->setFillColor(vuamber);
            dc->drawRect(drawThis,kDrawFilled);
            dc->setFillColor(vuamberNotch);
            for( auto pos = drawThis.left; pos <= drawThis.right; pos += notchDistance )
            {
               auto notch = CRect(CPoint(pos,drawThis.top),CPoint(1,drawThis.getHeight()));
               dc->drawRect( notch, kDrawFilled );
            }
            drawThis.right = ap;
         }
         dc->setFillColor( vugreen );

         dc->drawRect( drawThis, kDrawFilled );
         dc->setFillColor(vugreenNotch);
         for( auto pos = drawThis.left; pos <= drawThis.right; pos += notchDistance )
         {
           auto notch = CRect(CPoint(pos,drawThis.top),CPoint(1,drawThis.getHeight()));
           dc->drawRect( notch, kDrawFilled );
         }
      };

      drawMultiNotchedBar(vuBarLeft);
      if( stereo )
      {
         drawMultiNotchedBar(vuBarRight);
      }
   }

   dc->setFrameColor(skin->getColor(Colors::VuMeter::Background)); // the dark gray from original vector skin
   dc->setLineWidth(1);
   dc->drawGraphicsPath(borderRoundedRectPath, VSTGUI::CDrawContext::kPathStroked);

   borderRoundedRectPath->forget();
   dc->setDrawMode(origMode);

   setDirty(false);
}
