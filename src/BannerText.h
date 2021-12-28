

#ifndef __BannerText_H__
#define __BannerText_H__ 

/*  
 ~~~~~~~~~~~~~~~~
 Wordclock-Project
 ~~~~~~~~~~~~~~~~
© SERENE - Rene Lang 2021 (http://withstupid.net)
 
*/

class BannerText
{
private:
   String m_BannerHintText;
   String m_BannerText;
   int m_nextBannerShow;
public:
   void SetRuntimeBannerHintText(String s);
   void SetRuntimeBannerText(String s);
   const String & GetRuntimeBannerHintText(void);
   const String & GetRuntimeBannerText(void);
   bool ShowBannerTextIfRequired(void);   // returns true if Banner display
   BannerText();
   ~BannerText();
   
};

#endif 
