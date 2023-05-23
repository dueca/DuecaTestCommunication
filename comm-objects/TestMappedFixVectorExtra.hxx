};

template <size_t N, typename D>
void unPackData(dueca::AmorphReStore& s, dueca::fixvector<N, D>& v )
{ for (auto &e: v) { unPackData(s, e); } }

template <size_t N, typename D>
void packData(dueca::AmorphStore& s, const dueca::fixvector<N, D>& v )
{ for (const auto &e: v) { packData(s, e); } }

namespace {
