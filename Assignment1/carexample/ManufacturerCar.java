package carexample;

import java.util.ArrayList;
import java.util.List;

public class ManufacturerCar {
    public enum ProductionStatus {
        DESIGNED,
        IN_PRODUCTION,
        INSPECTED,
        RECALLED
    }

    private final String modelCode;
    private final String modelName;
    private String specification;
    private ProductionStatus productionStatus;
    private final double factoryPrice;
    private final List<String> producedVins;

    public ManufacturerCar(String modelCode, String modelName, double factoryPrice) {
        if (factoryPrice < 0) {
            throw new IllegalArgumentException("factoryPrice cannot be negative");
        }
        this.modelCode = modelCode;
        this.modelName = modelName;
        this.factoryPrice = factoryPrice;
        this.productionStatus = ProductionStatus.DESIGNED;
        this.producedVins = new ArrayList<String>();
    }

    public String getModelCode() { return modelCode; }
    public String getModelName() { return modelName; }
    public String getSpecification() { return specification; }
    public ProductionStatus getProductionStatus() { return productionStatus; }
    public double getFactoryPrice() { return factoryPrice; }

    public void designModel(String specification) {
        if (specification != null && !specification.trim().isEmpty()) {
            this.specification = specification;
            this.productionStatus = ProductionStatus.DESIGNED;
        }
    }

    public void startProduction() {
        if (productionStatus == ProductionStatus.DESIGNED) {
            productionStatus = ProductionStatus.IN_PRODUCTION;
        }
    }

    public boolean inspectQuality(String vin) {
        if (vin == null || vin.trim().isEmpty()
                || productionStatus == ProductionStatus.RECALLED) {
            return false;
        }
        if (!producedVins.contains(vin)) {
            producedVins.add(vin);
        }
        productionStatus = ProductionStatus.INSPECTED;
        return true;
    }

    public void updateSpecification(String specification) {
        if (productionStatus != ProductionStatus.IN_PRODUCTION
                && specification != null
                && !specification.trim().isEmpty()) {
            this.specification = specification;
        }
    }

    public double calculateCost() {
        return factoryPrice * 0.82;
    }

    public void recall(String vin) {
        if (vin != null && producedVins.remove(vin)) {
            productionStatus = ProductionStatus.RECALLED;
        }
    }

    public List<String> getProducedVins() {
        return new ArrayList<String>(producedVins);
    }
}
