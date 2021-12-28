#include <Arduino.h>
#include "BannerText.h"
#include "Scroller.h"
#define HINT_TIME_DELAY_MS  (1000*60*3) // alle 1 Minuten
bool StringIsNullOrEmpty(const String & s)
{
  if(s.length()==0) return true;
  String dummy = s;
  dummy.trim();
  if(dummy.length()==0) return true;
  return false;
}

BannerText::BannerText()
{
  m_nextBannerShow=-1;
}

BannerText::~BannerText()
{
}
void BannerText::SetRuntimeBannerHintText(String s)
{
  m_BannerHintText=s;
  m_nextBannerShow = StringIsNullOrEmpty(s) ? -1 : millis();
}
void BannerText::SetRuntimeBannerText(String s)
{
  m_BannerText=s;  
}
const String & BannerText::GetRuntimeBannerHintText(void)
{
  return m_BannerHintText;
}
const String & BannerText::GetRuntimeBannerText(void)
{
  return m_BannerText;
}

// returns true if Banner display
bool BannerText::ShowBannerTextIfRequired(void)   
{
  if(!StringIsNullOrEmpty(m_BannerText))
  {
    // immer anzeigen!!
    Scroller::ScrollerShowAndRun(m_BannerText,CRGB::Red);
    return true;
  }
  if(!StringIsNullOrEmpty(m_BannerHintText))
  {
    int now=millis();
    if(now > m_nextBannerShow)
    {
      // 1x durchlaufen.
      Scroller::ScrollerShowAndRun(m_BannerHintText,CRGB::Orange);
      m_nextBannerShow=millis()+HINT_TIME_DELAY_MS;
      return true;
    }
  }
  return false;

}






