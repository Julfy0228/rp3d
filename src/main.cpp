#include <iostream>
#include "EditorApp.h"

int main()
{
    EditorApp app;
    if (!app.init())
    {
        std::cerr << "Fatal error while app.init()" << std::endl;
        return -1;
    }

    app.run();

    return 0;
}
