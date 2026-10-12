package JingangNative10ParameterTableContract {
    private import Views::*;
    private import StandardViewDefinitions::*;
    private import JingangNative10ParameterAnalysisContract::*;

    rendering coreParameterTableRendering :> asElementTable {
        view parameterColumn :>> columnView { attribute label : String = "参数"; }
        view roleColumn :>> columnView { attribute label : String = "角色"; }
        view currentValueColumn :>> columnView { attribute label : String = "当前值"; }
        view unitColumn :>> columnView { attribute label : String = "单位"; }
        view targetColumn :>> columnView { attribute label : String = "目标"; }
        view marginColumn :>> columnView { attribute label : String = "余量"; }
        view statusColumn :>> columnView { attribute label : String = "状态"; }
        view sourceColumn :>> columnView { attribute label : String = "来源"; }
    }

    view def CoreParameterAnalysisTable :> GridView {
        render coreParameterTableRendering;
    }

    view coreParameterAnalysisTable : CoreParameterAnalysisTable {
        expose JingangNative10ParameterAnalysisContract::coreParameterAnalysis::totalMassResult;
        expose JingangNative10ParameterAnalysisContract::coreParameterAnalysis::enduranceResult;
        expose JingangNative10ParameterAnalysisContract::coreParameterAnalysis::peakElectricalPowerResult;
        expose JingangNative10ParameterAnalysisContract::coreParameterAnalysis::stopResponseTimeResult;
        render coreParameterTableRendering;
    }
}
