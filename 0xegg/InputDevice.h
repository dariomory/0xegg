#pragma once

class IMouseInput {
public:
    virtual ~IMouseInput() = default;

    virtual void LeftClick()                          = 0;
    virtual void RightClick()                         = 0;
    virtual void MoveTo(int x, int y)                 = 0;
    virtual void GetPosition(int& x, int& y)          = 0;
    virtual void Wait(int ms)                         = 0;
};

class IKeyboardInput {
public:
    virtual ~IKeyboardInput() = default;

    virtual void PressChar(char c)                    = 0;
    virtual bool AbortRequested()                     = 0;
};
