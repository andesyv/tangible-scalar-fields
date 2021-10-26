#ifndef MOLUMES_STLEXPORTER_H
#define MOLUMES_STLEXPORTER_H

#include "Interactor.h"

namespace molumes {
class STLExporter : public Interactor{
public:
    explicit STLExporter(Viewer * viewer);

    void keyEvent(int key, int scancode, int action, int mods) override;
    void display() override;

    void exportFile();

private:
    bool m_ctrl = false;
};
}


#endif //MOLUMES_STLEXPORTER_H
