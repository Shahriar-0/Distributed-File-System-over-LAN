#pragma once

#include <QByteArray>
#include <QRandomGenerator>
#include <vector>


#include <schifra_galois_field.hpp>
#include <schifra_reed_solomon.hpp>


static constexpr int   T_CORRECT   = 16;
static constexpr double NOISE_PROB = 0.01;
static constexpr int   FEC_N       = 255;
static constexpr int   FEC_K       = FEC_N - 2*T_CORRECT;

class NoiseAndFEC {
public:
  NoiseAndFEC()
    : gf_(8, schifra::galois::primitive_polynomial[8]),
      gen_(gf_, /* first root */ 1, /* parity symbols */ 2*T_CORRECT),
      encoder_(gf_, gen_),
      decoder_(gf_, gen_)
  {}

  QByteArray encode(const QByteArray &in) {
    QByteArray out;
    const char* ptr = in.constData();
    int remaining = in.size();
    while (remaining > 0) {
      int sz = qMin(remaining, FEC_K);
      std::vector<uint8_t> buf(FEC_N,0);
      memcpy(buf.data(), ptr, sz);
      encoder_.encode(buf.data());
      out.append(reinterpret_cast<char*>(buf.data()), FEC_N);
      ptr       += sz;
      remaining -= sz;
    }
    return out;
  }

  QByteArray injectNoise(const QByteArray &in) {
    QByteArray out = in;
    auto *rng = QRandomGenerator::global();
    for (int i = 0; i < out.size(); ++i) {
      uint8_t b = static_cast<uint8_t>(out[i]);
      for (int bit = 0; bit < 8; ++bit) {
        if (rng->generateDouble() < NOISE_PROB) b ^= uint8_t(1u<<bit);
      }
      out[i] = char(b);
    }
    return out;
  }

  QByteArray decode(const QByteArray &in, bool &corrupted) {
    corrupted = false;
    QByteArray out;
    const char* ptr = in.constData();
    int remaining = in.size();
    while (remaining >= FEC_N) {
      std::vector<uint8_t> buf(FEC_N);
      memcpy(buf.data(), ptr, FEC_N);
      if (!decoder_.decode(buf.data())) {
        corrupted = true;
        return {};
      }
      if (decoder_.corrected_errors() > 0) corrupted = true;
      out.append(reinterpret_cast<char*>(buf.data()), FEC_K);
      ptr       += FEC_N;
      remaining -= FEC_N;
    }
    return out;
  }

private:
  schifra::galois::field                     gf_;
  schifra::galois::field_generator_polynomial<> gen_;
  schifra::reed_solomon::encoder<>           encoder_;
  schifra::reed_solomon::decoder<>           decoder_;
};
