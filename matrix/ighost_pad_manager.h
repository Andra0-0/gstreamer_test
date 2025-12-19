#ifndef MATRIX_I_PAD_WRAPPER_H
#define MATRIX_I_PAD_WRAPPER_H

#include <gst/gst.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "noncopyable.h"

namespace mmx {

class IGhostPadManager;

using std::string;
using std::vector;
using std::unordered_map;
using IGhostPadManagerPtr = std::shared_ptr<IGhostPadManager>;

struct IGhostPadManager : public NonCopyable {
public:
  explicit IGhostPadManager(GstElement *bin, const char *prefix);

  ~IGhostPadManager();

  GstPad* add_pad(GstPad *tgt_pad);

  GstPad* get_pad(GstPad *tgt_pad);

  void del_pad(GstPad *tgt_pad);

  string get_info();

protected:
  size_t get_new_pad_id();

  GstPad* inter_add_pad(size_t id, GstPad *tgt_pad);
  void inter_del_pad(size_t id, GstPad *tgt_pad);

private:
  GstElement *bin_;
  string prefix_;
  vector<GstPad*> pad_vec_;
  unordered_map<GstPad*, gint> tgtpad_umap_;
  unordered_map<string, gint> padname_umap_;
};

} // namespace mmx

#endif // MATRIX_I_PAD_WRAPPER_H