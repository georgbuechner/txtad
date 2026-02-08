#ifndef SRC_UTILS_EVENTMANAGER_LISTNER_H
#define SRC_UTILS_EVENTMANAGER_LISTNER_H

#include "shared/utils/defines.h"
#include "shared/utils/parser/expression_parser.h"
#include "shared/utils/utils.h"
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

class Context;

class Listener {
  public:
    using Fn = std::function<void(std::string, std::string)>;
    
    // getter 
    virtual std::string id() const = 0;
    virtual std::string event() const = 0;
    virtual bool permeable() const = 0;
    virtual std::string arguments() const = 0;
    virtual std::string logic() const { 
      throw util::invalid_base_class_call("Listener::logic");
    }
    virtual std::string ctx_id () const { 
      throw util::invalid_base_class_call("invalid_base_class_call: Listener::ctx_id");
    }
    virtual std::weak_ptr<Context> ctx() const { 
      throw util::invalid_base_class_call("Listener::ctx");
    }
    virtual const nlohmann::json& original_json() const { 
      throw util::invalid_base_class_call("Listener::original_json");
    }

    // setter
    virtual void set_fn(Fn fn) {
      throw util::invalid_base_class_call("Listener::set_fn");
    }

    // methods
    virtual bool Test(const std::string& event, const ExpressionParser& parser) const = 0;
    virtual void Execute(std::string event) const = 0;
    virtual nlohmann::json json() const { 
      throw util::invalid_base_class_call("Listener::json");
    }
};

class LHandler : public Listener {
  public: 
    LHandler(std::string id, std::string re_event, Fn fn, bool permeable=true);
    
    // getter 
    std::string id() const override;
    std::string event() const override;
    bool permeable() const override;
    std::string arguments() const override;

    // setter 
    void set_fn(Fn fn) override;
    
    // methods 
    bool Test(const std::string& event, const ExpressionParser& parser) const override;

    void Execute(std::string event) const override;
    virtual nlohmann::json json() const override;

  protected: 
    const std::string _id;
    const util::Regex _event;
    std::string _arguments;
    Fn _fn; 
    const bool _permeable;

    // methods 

    /** 
     * Returns either the handlers arguments, the arguments with "#event"
     * replaced by the event, or only the event. 
     * @parem[in] event 
     * @return agrument, partyly replaced by event or only event
     */
    std::string ReplacedArguments(const std::string& event, const std::string& base) const;
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
    LForwarder(std::string id, std::string re_event, std::string arguments, bool permeable, 
        std::string logic="");

    LForwarder(const nlohmann::json& json, const nlohmann::json& original_json);

    // getter
    std::string logic() const override;
    const nlohmann::json& original_json() const { return _original_json; }

    // methods 
    bool Test(const std::string& event, const ExpressionParser& parser) const override;
    nlohmann::json json() const override;

    static void set_overwite_fn(Fn fn);

  protected: 
    static Fn _overwride_fn;
    const std::string _logic;
    nlohmann::json _original_json;
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

    LContextForwarder(const nlohmann::json& json, std::shared_ptr<Context> ctx, 
        const nlohmann::json& original_json);

    // getter
    std::string ctx_id() const override;
    std::weak_ptr<Context> ctx() const override;

    // methods 
    bool Test(const std::string& event, const ExpressionParser& parser) const override;
    nlohmann::json json() const override;

  private: 
    std::weak_ptr<Context> _ctx;
    const UseCtx _use_ctx_regex;

    static std::string GetCtxId(std::weak_ptr<Context> _ctx);
    static std::string GetCtxName(std::weak_ptr<Context> _ctx);
};

#endif
