#ifndef SRC_UTILS_EVENTMANAGER_LISTNER_H
#define SRC_UTILS_EVENTMANAGER_LISTNER_H

#include "shared/utils/defines.h"
#include "shared/utils/parser/expression_parser.h"
#include "shared/utils/utils.h"
#include <functional>
#include <memory>
#include <string>

class Context;

class Listener {
  public:
    using Fn = std::function<void(std::string, std::string)>;
    
    // getter 
    virtual std::string id() const = 0;
    virtual std::string event() const = 0;
    virtual bool permeable() const = 0;

    virtual bool Test(const std::string& event, const ExpressionParser& parser) = 0;
    virtual void Execute(std::string event) const = 0;
};

class LHandler : public Listener {
  public: 
    LHandler(std::string id, std::string re_event, Fn fn, bool permeable=true);
    
    // getter 
    std::string id() const override;
    std::string event() const override;
    bool permeable() const override;
    
    // methods 
    bool Test(const std::string& event, const ExpressionParser& parser) override;

    void Execute(std::string event) const override;

  protected: 
    const std::string _id;
    const util::Regex _event;
    std::string _arguments;
    Fn _fn; 
    const bool _permeable;
};

class LForwarder : public LHandler {
  public: 
    /**
     * Create new listener 
     * @param[in] id ( [Cat][num] f.e. M1 = default handlers, P1 -> higher priority, G1 -> lower prio
     * @param[in] event (gets regex-checked)
     * @param[in] arguments (passed when executing or replaced by first regex-match match was found)
     * @param[in] fn (function to execute fn(event:str, (agrument|match[0]):str))
     * @param[in] permeable (stop execution if not permeable)
     * @param[in] logic (additional evaluation)
     */
    LForwarder(std::string id, std::string re_event, std::string arguments, bool permeable, std::string logic="");

    LForwarder(const nlohmann::json& json);

    // methods 
    bool Test(const std::string& event, const ExpressionParser& parser) override;

    static void set_overwite_fn(Fn fn);

  private: 
    static Fn _overwride_fn;
    const std::string _logic;
};

class LContextForwarder : public LForwarder {
  public: 
    /**
     * Create new listener 
     * @param[in] id ( [Cat][num] f.e. M1 = default handlers, P1 -> higher priority, G1 -> lower prio
     * @param[in] event (gets regex-checked)
     * @param[in] arguments (passed when executing or replaced by first regex-match match was found)
     * @param[in] fn (function to execute fn(event:str, (agrument|match[0]):str))
     * @param[in] permeable (stop execution if not permeable)
     * @param[in] logic (additional evaluation)
     */
    LContextForwarder(std::string id, std::string re_event, std::weak_ptr<Context> ctx, std::string arguments, 
        bool permeable, UseCtx use_ctx_regex, std::string logic="");

    LContextForwarder(const nlohmann::json& json, std::shared_ptr<Context> ctx);

    // methods 
    bool Test(const std::string& event, const ExpressionParser& parser) override;

  private: 
    const std::string _logic;
    std::weak_ptr<Context> _ctx;
    const UseCtx _use_ctx_regex;

    static std::string GetCtxId(std::weak_ptr<Context> _ctx);
    static std::string GetCtxName(std::weak_ptr<Context> _ctx);
};

#endif
