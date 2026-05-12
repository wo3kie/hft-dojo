#include "./flat_list.hpp"
#include "./object_pool.hpp"
#include "./assert.hpp"
#include "./ring_buffer_spsc.hpp"

#include <cassert>
#include <array>
#include <chrono>
#include <iostream>
#include <limits>
#include <functional>
#include <utility>
#include <cassert>
#include <iostream>
#include <limits>

#if defined(__GNUC__) || defined(__clang__)
  #define LIKELY(x)   (__builtin_expect(!!(x), 1))
  #define UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
  #define LIKELY(x)   (x)
  #define UNLIKELY(x) (x)
#endif

int I = 0;

namespace bl /* branchless */ {

template<std::signed_integral T>
inline T min0(T x) noexcept {
    return x & (x >> (sizeof(T)*8 - 1));
}

template<std::signed_integral T>
inline T max0(T x) noexcept {
    return x & ~(x >> (sizeof(T)*8 - 1));
}

template<std::signed_integral T>
inline T min(T x, T y) noexcept {
    return y ^ ((x ^ y) & -(x < y));
}

template<std::signed_integral T>
inline T max(T x, T y) noexcept {
    return x ^ ((x ^ y) & -(x < y));
}

template<std::signed_integral T>
inline T clamp(T x, T min, T max) noexcept {
    return min + ((x - min) & -((x - min) < 0)) + ((max - x) & -((max - x) < 0));
}

template<std::signed_integral T>
bool in_range(T x, T min, T max) noexcept {
    return (x >= min) && (x <= max);
}

template<std::signed_integral T>
int32_t abs(T x) noexcept {
    return (x + (x >> (sizeof(T) * 8 - 1))) ^ (x >> (sizeof(T) * 8 - 1));
}

} // namespace bl

enum EventType : int32_t
{
  ETrade = 1,

  ECreateAccepted,
  ECreateRejected,
  
  EUpdateAccepted,
  EUpdateRejected,

  ECancelAccepted,
  ECancelRejected,

  EOrderExpired,

};

struct Order
{
  int32_t id;
  int32_t qty;
};

struct Event
{
  int32_t m1{-1};
  int32_t m2{-1};
  int32_t m3{-1};
  int32_t m4{-1};
};

bool operator==(const Event& lhs, const Event& rhs)
{
  return lhs.m1 == rhs.m1 && lhs.m2 == rhs.m2 && lhs.m3 == rhs.m3 && lhs.m4 == rhs.m4;
}

Event Trade(int32_t price, int32_t qty, int32_t maker, int32_t taker)
{
  return {.m1 = price, .m2 = qty, .m3 = maker, .m4 = taker};
}

Event CreateAccepted(int32_t id, int32_t slot)
{
  return {.m1 = EventType::ECreateAccepted, .m2 = id, .m3 = slot};
}

Event CreateRejected(int32_t id, int32_t size)
{
  return {.m1 = EventType::ECreateRejected, .m2 = id, .m3 = size};
}

Event CancelAccepted(int32_t id)
{
  return {.m1 = EventType::ECancelAccepted, .m2 = id};
}

Event CancelRejected(int32_t id)
{
  return {.m1 = EventType::ECancelRejected, .m2 = id};
}

Event UpdateAccepted(int32_t id)
{
  return {.m1 = EventType::EUpdateAccepted, .m2 = id};
}

Event UpdateRejected(int32_t id, int32_t size)
{
  return {.m1 = EventType::EUpdateRejected, .m2 = id, .m3 = size};
}

Event OrderExpired(int32_t id)
{
  return {.m1 = EventType::EOrderExpired, .m2 = id};
}

std::ostream& operator<<(std::ostream& os, const Event& event)
{
  if (event.m4 != -1) {
    return os << "Trade: price=" << event.m1 << ", qty=" << event.m2 << ", maker=" << event.m3 << ", taker=" << event.m4;
  }
  
  if (event.m1 == EventType::ECreateAccepted) {
    return os << "CreateAccepted: id=" << event.m2 << ", slot=" << event.m3;
  }
  
  if (event.m1 == EventType::ECreateRejected) {
    return os << "CreateRejected: id=" << event.m2 << ", size=" << event.m3;
  }
  
  if (event.m1 == EventType::EUpdateAccepted) {
    return os << "UpdateAccepted: id=" << event.m2;
  }
  
  if (event.m1 == EventType::EUpdateRejected) {
    return os << "UpdateRejected: id=" << event.m2;
  }
  
  if (event.m1 == EventType::ECancelAccepted) {
    return os << "CancelAccepted: id=" << event.m2;
  }
  
  if (event.m1 == EventType::ECancelRejected) {
    return os << "CancelRejected: id=" << event.m2;
  }
  
  if (event.m1 == EventType::EOrderExpired) {
    return os << "OrderExpired: id=" << event.m2;
  }

  return os << "UnknownEvent: m1=" << event.m1 << ", m2=" << event.m2 << ", m3=" << event.m3 << ", m4=" << event.m4;
}

struct PriceLevel
{
  FlatList<32, Order> orders;
};


template<typename Type, int32_t Size>
struct _Array
{
  static_assert((Size & (Size - 1)) == 0);

public:
  Type& operator[](int32_t index)
  {
    index = index & (Size - 1);
    return _data[index];
  }

  const Type& operator[](int32_t index) const
  {
    index = index & (Size - 1);
    return _data[index];
  }

  std::array<Type, Size>& data()
  {
    return _data;
  }

  const std::array<Type, Size>& data() const
  {
    return _data;
  }

private:
  std::array<Type, Size> _data;
};


template<int32_t Levels>
struct PriceLevels
{
  static_assert((Levels & (Levels + 1)) == 0);

  PriceLevels(int32_t centerPrice = 0, int32_t centerIndex = 0)
    : _centerPrice(centerPrice)
    , _centerIndex(centerIndex)
  {
  }

  int32_t centerPrice() const
  {
    return _centerPrice;
  }

  int32_t levels() const {
    return Levels;
  }

  int32_t minPrice() const
  {
    return bl::max0(_centerPrice - Levels);
  }

  int32_t maxPrice() const
  {
    return _centerPrice + Levels;
  }

  PriceLevel& index(int32_t index)
  {
    index += _centerIndex;
    return _levels[index];
  }

  const PriceLevel& index(int32_t index) const
  {
    index += _centerIndex;
    return _levels[index];
  }

  PriceLevel& price(int32_t price)
  {
    price -= _centerPrice;
    price += _centerIndex;
    return _levels[price];
  }

  const PriceLevel& price(int32_t price) const
  {
    price -= _centerPrice;
    price += _centerIndex;
    return _levels[price];
  }

  int32_t priceToIndex(int32_t price) const
  {
    price -= _centerPrice;
    return price;
  }

  void shiftUp() {
    _centerPrice += 1;
    _centerIndex += 1;
    _centerIndex &= (Levels - 1);
  }

  void shiftDown() {
    _centerPrice -= 1;
    _centerIndex -= 1;
    _centerIndex &= (Levels - 1);
  }

private:
  int32_t _centerPrice;
  int32_t _centerIndex;

  _Array<PriceLevel, Levels + 1 + Levels + 1> _levels;
};

template<int32_t Levels>
struct OrderBook
{
  static_assert((Levels & (Levels + 1)) == 0);

  int32_t _sellTopPrice;
  int32_t _buyTopPrice;

  PriceLevels<Levels> _sellLevels;
  PriceLevels<Levels> _buyLevels;

  RingBufferSPSC<Event, 1024>& _bufferOut;

  OrderBook(RingBufferSPSC<Event, 1024>& bufferOut, int32_t centerPrice)
    : _sellTopPrice{-1}
    , _buyTopPrice{-1}
    , _buyLevels(centerPrice)
    , _sellLevels(centerPrice)
    , _bufferOut(bufferOut)
  {
  }

  int32_t centerPrice() const
  {
    return _buyLevels.centerPrice();
  }

  void emitEvent(Event event)
  {
    while(_bufferOut.push(event) == false) {
    }
  }

  int32_t buyPriceFrom() const
  {
    return _buyTopPrice;
  }

  int32_t buyPriceTo(int32_t price) const
  {
    int32_t min_price = _buyLevels.minPrice();
    int32_t max_price = bl::max(price, min_price);
    return max_price;
  }

  int32_t buyIndexFrom() const
  {
    int32_t index = _buyLevels.priceToIndex(_buyTopPrice);
    return index;
  }

  int32_t sellPriceFrom() const
  {
    return _sellTopPrice;
  }

  int32_t sellPriceTo(int32_t price) const
  {
    int32_t maxPrice = _sellLevels.maxPrice();
    int32_t minPrice = bl::min(price, maxPrice);
    return minPrice;
  }

  int32_t sellIndexFrom() const
  {
    int32_t index = _sellLevels.priceToIndex(_sellTopPrice);
    return index;
  }

  void insertSellOrder(int32_t orderId, int32_t price, int32_t qty) {
    PriceLevel& level = _sellLevels.price(price);
    int32_t slot = level.orders.push_back({orderId, qty});

    if(UNLIKELY(slot == -1)) {
      return emitEvent(CreateRejected(orderId, qty));
    }

    emitEvent(CreateAccepted(orderId, slot));

    auto clearSignBit = [](int32_t i) -> int32_t {
      return i & 0x7FFFFFFF;
    };

    _sellTopPrice = bl::min(clearSignBit(_sellTopPrice), price);
  }

  void insertBuyOrder(int32_t orderId, int32_t price, int32_t qty) {
    PriceLevel& level = _buyLevels.price(price);
    int32_t slot = level.orders.push_back({orderId, qty});

    if(UNLIKELY(slot == -1)) {
      return emitEvent(CreateRejected(orderId, qty));
    }

    emitEvent(CreateAccepted(orderId, slot));
    _buyTopPrice = bl::max(_buyTopPrice, price);
  }

  void shiftUp() {
    {
      int32_t minPrice = _buyLevels.minPrice();
      PriceLevel& level = _buyLevels.price(minPrice);
      expireOrders(level);
      _buyLevels.shiftUp();
    }

    {
      int32_t minPrice = _sellLevels.minPrice();
      PriceLevel& level = _sellLevels.price(minPrice);
      expireOrders(level);
      _sellLevels.shiftUp();
    }
  }

  void shiftDown() {
    {
      int32_t maxPrice = _buyLevels.maxPrice();
      PriceLevel& level = _buyLevels.price(maxPrice);
      expireOrders(level);
      _buyLevels.shiftDown();
    }

    {
      int32_t maxPrice = _sellLevels.maxPrice();
      PriceLevel& level = _sellLevels.price(maxPrice);
      expireOrders(level);
      _sellLevels.shiftDown();
    }
  }

  void expireOrders(PriceLevel& level) {
    while(level.orders.empty() == false) {
      Order& order = level.orders.front();
      
      emitEvent(OrderExpired(order.id));
      
      order.id = -1;
      level.orders.pop_front();
    }
  }
};

template<int32_t Levels, int32_t LevelsTolerance = Levels + 1>
struct MatchingEngine 
{
  RingBufferSPSC<Event, 1024> _bufferOut;
  OrderBook<Levels> _orderBook;

  MatchingEngine(int32_t centerPrice)
    : _bufferOut()
    , _orderBook(std::ref(_bufferOut), centerPrice)
  {
  }

public:
  void insertSellOrderPL(int32_t orderId, int32_t price, int32_t qty)
  {
    int32_t minPrice = _orderBook._buyLevels.minPrice();
    int32_t maxPrice = _orderBook._buyLevels.maxPrice();
    
    if (UNLIKELY(! bl::in_range(price, minPrice, maxPrice))) {
      return emitEvent(CreateRejected(orderId, qty));
    }

    qty = tradeSell(orderId, price, qty);
    
    if (UNLIKELY(qty != 0)) {
      _orderBook.insertSellOrder(orderId, price, qty);
    }
  }

  void insertSellOrderMKT(int32_t orderId, int32_t qty)
  {
    qty = tradeSell(orderId, 0, qty);
    
    if (qty != 0) {
      emitEvent(CreateRejected(orderId, qty));
    }
  }

  void insertBuyOrderPL(int32_t orderId, int32_t price, int32_t qty)
  {
    int32_t minPrice = _orderBook._buyLevels.minPrice();
    int32_t maxPrice = _orderBook._buyLevels.maxPrice();
    
    if (UNLIKELY(! bl::in_range(price, minPrice, maxPrice))) {
      return emitEvent(CreateRejected(orderId, qty));
    }

    qty = tradeBuy(orderId, price, qty);
    
    if (UNLIKELY(qty != 0)) {
      _orderBook.insertBuyOrder(orderId, price, qty);
    }
  }

  void insertBuyOrderMKT(int32_t orderId, int32_t qty)
  {
    qty = tradeBuy(orderId, 9999, qty);
    
    if (qty != 0) {
      emitEvent(CreateRejected(orderId, qty));
    }
  }

  void updateBuyOrder(int32_t orderId, int32_t slot, int32_t price, int32_t qty)
  {
    int32_t minPrice = _orderBook._buyLevels.minPrice();
    int32_t maxPrice = _orderBook._buyLevels.maxPrice();
    
    if (UNLIKELY(! bl::in_range(price, minPrice, maxPrice))) {
      return emitEvent(UpdateRejected(orderId, qty));
    }

    PriceLevel& priceLevel = _orderBook._buyLevels.price(price);
    Order& order = priceLevel.orders.at(slot);

    if (UNLIKELY(order.id != orderId)) {
      return emitEvent(UpdateRejected(orderId, qty));
    }
    
    order.qty = qty;
    tradeSell(orderId, price, qty);
    
    emitEvent(UpdateAccepted(orderId));
  }

  void updateSellOrder(int32_t orderId, int32_t slot, int32_t price, int32_t qty)
  {
    int32_t minPrice = _orderBook._sellLevels.minPrice();
    int32_t maxPrice = _orderBook._sellLevels.maxPrice();
    
    if (UNLIKELY(! bl::in_range(price, minPrice, maxPrice))) {
      return emitEvent(UpdateRejected(orderId, qty));
    }

    PriceLevel& priceLevel = _orderBook._sellLevels.price(price);
    Order& order = priceLevel.orders.at(slot);

    if (UNLIKELY(order.id != orderId)) {
      return emitEvent(UpdateRejected(orderId, qty));
    }
    
    order.qty = qty;
    tradeBuy(orderId, price, qty);

    emitEvent(UpdateAccepted(orderId));
  }

  RingBufferSPSC<Event, 1024>& bufferOut()
  {
    return _bufferOut;
  }

private:
  void emitEvent(Event event)
  {
    while(_bufferOut.push(event) == false) {
    }
  }

  void shiftOrderBook(int32_t price) {
    int32_t diff = price - _orderBook.centerPrice();

    if (bl::abs(diff) <= LevelsTolerance) {
      return;
    }

    if (diff < 0) {
      while(_orderBook.centerPrice() != price) {
        _orderBook.shiftDown();
      }
    } else {
      while(_orderBook.centerPrice() != price) {
        _orderBook.shiftUp();
      }
    }
  }

  int32_t tradeAtPriceLevel(int32_t orderId, int32_t price, int32_t qty, PriceLevel& level)
  {
    while(qty > 0 && ! level.orders.empty()) {
      Order& otherOrder = level.orders.front();
      int32_t tradeQty = std::min(qty, otherOrder.qty);
      
      qty -= tradeQty;
      otherOrder.qty -= tradeQty;
      
      emitEvent(Trade(price, tradeQty, orderId, otherOrder.id));
      
      if(otherOrder.qty == 0) {
        level.orders.front().id = -1;
        level.orders.pop_front();
      }
    }

    return qty;
  }

  int32_t tradeSell(int32_t orderId, int32_t priceTo, int32_t qty)
  {
    if (UNLIKELY(_orderBook._buyTopPrice == -1)) {
      return qty;
    }

    {
      priceTo = _orderBook.buyPriceTo(priceTo);

      int32_t price = _orderBook.buyPriceFrom();
      int32_t index = _orderBook.buyIndexFrom();
      
      while (price >= priceTo) {       
        if (qty == 0) {
          break;
        }
        
        PriceLevel& level = _orderBook._buyLevels.index(index);
        qty = tradeAtPriceLevel(orderId, price, qty, level);
        
        int32_t empty = (int32_t)level.orders.empty();
        _orderBook._buyTopPrice -= empty;
        
        shiftOrderBook(price);

        price -= 1;
        index -= 1;
      }
      
      return qty;
    }
  }

  int32_t tradeBuy(int32_t orderId, int32_t priceTo, int32_t qty)
  {
    if (UNLIKELY(_orderBook._sellTopPrice == -1)) {
      return qty;
    }

    {
      priceTo = _orderBook.sellPriceTo(priceTo);

      int32_t price = _orderBook.sellPriceFrom();
      int32_t index = _orderBook.sellIndexFrom();
      
      while (price <= priceTo) {
        if (qty == 0) {
          break;
        }
        
        PriceLevel& level = _orderBook._sellLevels.index(index);
        qty = tradeAtPriceLevel(orderId, price, qty, level);
        
        int32_t empty = (int32_t)level.orders.empty();
        _orderBook._sellTopPrice += empty;
        
        shiftOrderBook(price);

        price += 1;
        index += 1;
      }
      
      return qty;
    }
  }
};

void test_simple_transaction()
{
  {
    MatchingEngine<3> engine(100);
    
    engine.insertBuyOrderPL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));
    
    engine.insertSellOrderPL(2, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == Trade(100, 200, 2, 1));
  }

  {
    MatchingEngine<3> engine(100);
    
    engine.insertSellOrderPL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));
    
    engine.insertBuyOrderPL(2, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == Trade(100, 200, 2, 1));
  }
}

void test_partial_fill()
{
  {
    MatchingEngine<3> engine(100);
    
    engine.insertBuyOrderPL(1, /* price */ 100, /* qty */ 180);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));
    
    engine.insertSellOrderPL(2, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == Trade(100, 180, 2, 1));
  }

  {
    MatchingEngine<3> engine(100);
    
    engine.insertSellOrderPL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));
    
    engine.insertBuyOrderPL(2, /* price */ 100, /* qty */ 180);
    Assert(engine.bufferOut().pop() == Trade(100, 180, 2, 1));
  }
}

void test_single_price_level()
{
  {
    MatchingEngine<3> engine(100);
    
    engine.insertBuyOrderPL(1, /* price */ 100, /* qty */ 180);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));
    
    engine.insertBuyOrderPL(2, /* price */ 100, /* qty */ 20);
    Assert(engine.bufferOut().pop() == CreateAccepted(2, 1));
    
    engine.insertSellOrderPL(3, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == Trade(100, 180, 3, 1));
    Assert(engine.bufferOut().pop() == Trade(100, 20, 3, 2));
  }

  {
    MatchingEngine<3> engine(100);
    
    engine.insertSellOrderPL(1, /* price */ 100, /* qty */ 200);
    Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));
    
    engine.insertSellOrderPL(2, /* price */ 100, /* qty */ 20);
    Assert(engine.bufferOut().pop() == CreateAccepted(2, 1));
    
    engine.insertBuyOrderPL(3, /* price */ 100, /* qty */ 220);
    Assert(engine.bufferOut().pop() == Trade(100, 200, 3, 1));
    Assert(engine.bufferOut().pop() == Trade(100, 20, 3, 2));
  }
}

void test_insert_level()
{
  MatchingEngine<3> engine(100);
  
  engine.insertBuyOrderPL(1, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(1, 0));

  engine.insertBuyOrderPL(2, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(2, 1));

  engine.insertBuyOrderPL(3, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(3, 2));

  engine.insertBuyOrderPL(4, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(4, 3));

  engine.insertBuyOrderPL(5, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(5, 4));

  engine.insertBuyOrderPL(6, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(6, 5));

  engine.insertBuyOrderPL(7, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(7, 6));

  engine.insertBuyOrderPL(8, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(8, 7));

  engine.insertBuyOrderPL(9, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateRejected(9, 100));
}

void test_insert_levels()
{
  MatchingEngine<3> engine(100);
  
  engine.insertBuyOrderPL(1, /* price */ 96, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateRejected(1, 100));

  engine.insertBuyOrderPL(2, /* price */ 97, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(2, 0));

  engine.insertBuyOrderPL(3, /* price */ 98, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(3, 0));

  engine.insertBuyOrderPL(4, /* price */ 99, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(4, 0));

  engine.insertBuyOrderPL(5, /* price */ 100, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(5, 0));

  engine.insertBuyOrderPL(6, /* price */ 101, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(6, 0));

  engine.insertBuyOrderPL(7, /* price */ 102, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(7, 0));

  engine.insertBuyOrderPL(8, /* price */ 103, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateAccepted(8, 0));

  engine.insertBuyOrderPL(9, /* price */ 104, /* qty */ 100);
  Assert(engine.bufferOut().pop() == CreateRejected(9, 100));
}

void bench() 
{
  MatchingEngine<3> engine(100);

  I = 0;

  int32_t orderId = 0;
  int32_t minPrice = 96;
  int32_t maxPrice = 104;

  for (int32_t i = 0; i < 1000; ++i) {
    for (int32_t p = minPrice; p <= maxPrice; ++p) {
      for(int32_t o = 0; o < 32 ; ++o) {
        engine.insertBuyOrderPL(orderId++, p, 100);
      }

      engine.insertSellOrderMKT(orderId++, 32 * 100);
    }
  }
}

#include "./timer.hpp"

int main()
{

#ifdef NDEBUG
  timer( [](){ bench(); }, 1000 ).log([&](long int ns, const std::string& formatted) {
    std::cout << "Bench took " << formatted << " (" << ns << "ns) for " << I << " events" << std::endl;
  });
#else
  test_simple_transaction();
  test_partial_fill();
  test_single_price_level();
  test_insert_levels();
  
  // test_many_price_levels();
#endif

  return 0;
}
