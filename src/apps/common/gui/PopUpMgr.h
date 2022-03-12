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
            if(index < size && index >= -1)
            {
                cur = index;
                popUps[cur].SetVisible(true);
            }
        }

        void DrawCur()
        {
            if(cur != -1)
                popUps[cur].Draw();
        }

    private:
        int cur = -1;
        T popUps[size];
    };
}