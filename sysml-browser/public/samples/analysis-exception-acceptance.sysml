package AnalysisExceptionAcceptance {
    calc def IdentityDuration {
        in durationInput :> ISQ::duration;
        return durationOutput :> ISQ::duration = durationInput;
    }

    part def ExceptionInputs {
        attribute healthyDuration :> ISQ::duration = 2[h];
        attribute massAsDuration :> ISQ::mass = 2[h];
        attribute zero : Real = 0;
    }

    part exceptionInputs : ExceptionInputs;

    analysis exceptionStatusAnalysis {
        subject = exceptionInputs;

        // 正常结果用于证明一个计算失败不会阻断其他结果。
        attribute healthyResult :> ISQ::duration =
            IdentityDuration(exceptionInputs.healthyDuration);

        // 五类异常状态的最小、可重复模型。
        attribute missingInputResult : Real = exceptionInputs.notDeclared;
        attribute unitErrorResult :> ISQ::mass = exceptionInputs.massAsDuration;
        attribute unsupportedResult : Real = UnknownCalculation(exceptionInputs.zero);
        attribute cycleResultA : Real = cycleResultB + 1;
        attribute cycleResultB : Real = cycleResultA + 1;
        attribute runtimeErrorResult : Real = 1 / exceptionInputs.zero;

        constraint healthyTarget {
            healthyResult >= 2[h]
        }
    }

    rendering exceptionStatusTableRendering :> asElementTable {
        view parameterColumn :>> columnView { attribute label : String = "参数"; }
        view roleColumn :>> columnView { attribute label : String = "角色"; }
        view currentValueColumn :>> columnView { attribute label : String = "当前值"; }
        view unitColumn :>> columnView { attribute label : String = "单位"; }
        view statusColumn :>> columnView { attribute label : String = "状态"; }
        view sourceColumn :>> columnView { attribute label : String = "来源"; }
    }

    view def ExceptionStatusTable :> GridView {
        render exceptionStatusTableRendering;
    }

    view exceptionStatusTable : ExceptionStatusTable {
        expose AnalysisExceptionAcceptance::exceptionStatusAnalysis::healthyResult;
        expose AnalysisExceptionAcceptance::exceptionStatusAnalysis::missingInputResult;
        expose AnalysisExceptionAcceptance::exceptionStatusAnalysis::unitErrorResult;
        expose AnalysisExceptionAcceptance::exceptionStatusAnalysis::unsupportedResult;
        expose AnalysisExceptionAcceptance::exceptionStatusAnalysis::cycleResultA;
        expose AnalysisExceptionAcceptance::exceptionStatusAnalysis::runtimeErrorResult;
        render exceptionStatusTableRendering;
    }
}
