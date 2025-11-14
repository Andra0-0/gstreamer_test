#include "in_device_module.h"

InDeviceModule::InDeviceModule(GstElement *matrix_pipe)
{

}
InDeviceModule::~InDeviceModule()
{

}

gint InDeviceModule::add_indev(const InDeviceConfig &config)
{

}

gint InDeviceModule::add_indev_many(const list<InDeviceConfig> &configs)
{

}

gint InDeviceModule::del_indev(gchar *name)
{

}

gint InDeviceModule::find_indev(const gchar *name, shared_ptr<InputDevice> &indev)
{

}