#pragma once

#include <PopUp.h>

namespace GUI
{
    template<int size>
    class PopUpMgr
    {
    public:
        inline void Set(int index, std::unique_ptr<PopUp> popUp)
        {
            if(index < size && index >= 0)
                popUps[index] = std::move(popUp);
        }

        inline void Open(int index)
        {
            for(auto i = 0; i < size; i++)
            {
                popUps[i]->SetVisible(false);
            }

            if(index < size && index >= 0)
            {
                cur = index;
                popUps[cur]->Open();
            }
        }

        inline void Close()
        {
            for(auto i = 0; i < size; i++)
            {
                popUps[i]->Close();
            }
        }

        inline bool IsOpen()
        {
            bool open = false;
            for(auto i = 0; i < size; i++)
            {
                if(popUps[i]->IsVisible())
                    return true;
            }
            return false;
        }

        inline void DrawCur()
        {
            for(auto i = 0; i < size; i++)
            {
                popUps[i]->Draw();
            }
        }

    private:
        int cur = -1;
        std::unique_ptr<PopUp> popUps[size];
    };
}