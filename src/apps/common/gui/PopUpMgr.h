#pragma once

#include <PopUp.h>

namespace GUI
{
    template<typename T, int size>
    class PopUpMgr
    {
    public:
        void Set(int index, T popUp)
        {
            if(index < size && index >= 0)
                popUps[index] = popUp;
        }

        void SetCur(int index)
        {
            for(auto i = 0; i < size; i++)
            {
                popUps[i].SetVisible(false);
            }

            if(index < size && index >= -1)
            {
                cur = index;
                popUps[cur].SetVisible(true);
            }
        }

        void DrawCur()
        {
            for(auto i = 0; i < size; i++)
            {
                popUps[i].Draw();
            }
        }

    private:
        int cur = -1;
        T popUps[size];
    };
}