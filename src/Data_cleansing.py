# ===================================================================
# ED YASA STATION DATA WRANGLING AND ANALYSIS
# Based on Ucaneo Data Cleansing Philosophy
# ===================================================================

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from google.colab import auth
import gspread
from google.auth import default
from datetime import datetime
import os
import glob

# Google Authentication
auth.authenticate_user()
creds, _ = default()
gc = gspread.authorize(creds)

# Configuration based on ED Data Inventory
ED_INVENTORY_SHEET_ID = '10eTbYCrdJR3daXgbOoC74INd-T_ETsE91bA9Xn7Fvxg'
YASA_DATA_FOLDER = '15TLkwaY4xdhq6qnQq5k-8sy3L3ZWZ3WU'

# ===============================================
# STEP 1: READ ED DATA INVENTORY
# ===============================================

def read_ed_inventory():
    """Read the ED Data Inventory sheet to get experiment metadata"""
    try:
        sheet = gc.open_by_key(ED_INVENTORY_SHEET_ID)
        
        # Read Sheet 1: Data inventory
        inventory_ws = sheet.get_worksheet(0)
        inventory_data = inventory_ws.get_all_records()
        
        # Read Sheet 2: Standard settings
        settings_ws = sheet.get_worksheet(1)
        standard_settings = settings_ws.get_all_records()
        
        return pd.DataFrame(inventory_data), pd.DataFrame(standard_settings)
    except Exception as e:
        print(f"Error reading ED inventory: {e}")
        return None, None

# ===============================================
# STEP 2: YASA DATA WRANGLING
# ===============================================

def recognize_yasa_file_type(df):
    """Recognize YASA file type by header structure"""
    headers = df.columns.tolist()
    
    # File 1 & 2 recognition pattern
    if 'Temperature(°C) Base IN' in headers or 'Cond(mS/cm) Acid' in headers:
        if 'pH(pH) Base IN' in headers:
            return 'pH_conductivity_file'
    
    # File 3 recognition pattern
    if any('M_500SCCM' in col for col in headers):
        return 'mass_flow_file'
    
    # File 4 recognition pattern
    if 'Volts' in headers and 'Amps' in headers:
        return 'voltage_current_file'
    
    return 'unknown'

def read_yasa_experiment_data(experiment_folder):
    """Read and wrangle YASA experiment data from multiple files"""
    
    data_files = {
        'pH_conductivity': None,
        'mass_flow': None,
        'voltage_current': None
    }
    
    # Get all CSV files in the experiment folder
    csv_files = glob.glob(os.path.join(experiment_folder, '*.csv'))
    
    for file_path in csv_files:
        try:
            df = pd.read_csv(file_path)
            file_type = recognize_yasa_file_type(df)
            
            if file_type == 'pH_conductivity_file':
                data_files['pH_conductivity'] = df
            elif file_type == 'mass_flow_file':
                data_files['mass_flow'] = df
            elif file_type == 'voltage_current_file':
                data_files['voltage_current'] = df
        except Exception as e:
            print(f"Error reading {file_path}: {e}")
    
    # Align all data by time (standard is measurement every 3 seconds)
    return align_yasa_data_by_time(data_files)

def align_yasa_data_by_time(data_files):
    """Align all YASA data files by timestamp"""
    
    # Create master dataframe with 3-second intervals
    if data_files['voltage_current'] is not None:
        master_df = data_files['voltage_current'].copy()
        master_df['Time_seconds'] = master_df.index * 3  # 3 seconds per measurement
        
        # Merge pH/conductivity data
        if data_files['pH_conductivity'] is not None:
            ph_cond_df = data_files['pH_conductivity'].copy()
            ph_cond_df['Time_seconds'] = ph_cond_df.index * 3
            
            # Merge on time
            master_df = pd.merge(master_df, ph_cond_df, on='Time_seconds', how='outer')
        
        # Merge mass flow data
        if data_files['mass_flow'] is not None:
            mass_flow_df = data_files['mass_flow'].copy()
            mass_flow_df['Time_seconds'] = mass_flow_df.index * 3
            
            # Extract CO2 flow column
            co2_cols = [col for col in mass_flow_df.columns if 'CO2' in col or 'mass_flow' in col.lower()]
            if co2_cols:
                master_df = pd.merge(master_df, mass_flow_df[['Time_seconds'] + co2_cols], 
                                   on='Time_seconds', how='outer')
        
        # Convert time to minutes for plotting
        master_df['Time_minutes'] = master_df['Time_seconds'] / 60
        
        return master_df
    
    return None

# ===============================================
# STEP 3: DATA PROCESSING BASED ON ANALYSIS TYPE
# ===============================================

def process_current_density_screening(df, experiment_metadata):
    """Process data for current density screening analysis"""
    
    # Get standard settings from metadata
    cell_area = experiment_metadata.get('A_stack', 0.112)  # m²
    
    # Calculate current density
    if 'Amps' in df.columns:
        df['Current_Density_mA_cm2'] = df['Amps'] * 1000 / (cell_area * 10000)  # Convert to mA/cm²
    
    # Identify current density steps
    cd_steps = identify_current_density_steps(df)
    
    # Calculate averages for last 10 minutes of each step
    results = {}
    for step_info in cd_steps:
        last_10_min_mask = (df['Time_minutes'] >= step_info['end_time'] - 10) & \
                          (df['Time_minutes'] <= step_info['end_time'])
        
        step_data = df[last_10_min_mask]
        
        results[step_info['current_density']] = {
            'avg_voltage': step_data['Volts'].mean() if 'Volts' in step_data else np.nan,
            'avg_co2_flow': step_data[[col for col in step_data.columns if 'CO2' in col][0]].mean() 
                           if any('CO2' in col for col in step_data.columns) else np.nan,
            'data_points': len(step_data)
        }
    
    return df, results

def identify_current_density_steps(df):
    """Identify current density steps in the data"""
    
    if 'Current_Density_mA_cm2' not in df.columns:
        return []
    
    # Find step changes
    cd_diff = df['Current_Density_mA_cm2'].diff().abs()
    step_changes = df[cd_diff > 2].index.tolist()
    
    # Add start and end
    step_changes = [0] + step_changes + [len(df)-1]
    
    steps = []
    for i in range(len(step_changes)-1):
        start_idx = step_changes[i]
        end_idx = step_changes[i+1] if i < len(step_changes)-2 else len(df)-1
        
        avg_cd = df.iloc[start_idx:end_idx]['Current_Density_mA_cm2'].mean()
        
        steps.append({
            'current_density': round(avg_cd / 5) * 5,  # Round to nearest 5
            'start_time': df.iloc[start_idx]['Time_minutes'],
            'end_time': df.iloc[end_idx]['Time_minutes']
        })
    
    return steps

# ===============================================
# STEP 4: INTEGRATE WITH EXISTING PLOTTING CODE
# ===============================================

def create_ed_yasa_plots(df, analysis_results):
    """Create all required plots with real data"""
    
    # Your existing plotting code here, but now using real data
    # Plot 1: Voltage vs Current Density vs Time
    # Plot 2: CO2 Flow Rate
    # Plot 3: Specific Energy Consumption
    # Plot 4: pH vs Time
    # Plot 5: Conductivity vs Time
    
    pass  # Implementation continues with your existing plotting code

# ===============================================
# MAIN EXECUTION
# ===============================================

if __name__ == "__main__":
    # Step 1: Read experiment metadata
    inventory_df, settings_df = read_ed_inventory()
    
    # Step 2: Select experiment to analyze
    # For example, get the latest YASA experiment
    yasa_experiments = inventory_df[inventory_df['Instrument'] == 'Yasa']
    
    if not yasa_experiments.empty:
        latest_experiment = yasa_experiments.iloc[-1]
        experiment_folder = latest_experiment['Date of experiment']
        
        # Step 3: Read and wrangle raw data
        raw_data = read_yasa_experiment_data(experiment_folder)
        
        # Step 4: Process based on analysis type
        if latest_experiment['Type of experiment'] == 'Current density screening':
            processed_data, results = process_current_density_screening(
                raw_data, 
                latest_experiment.to_dict()
            )
            
            # Step 5: Create plots
            create_ed_yasa_plots(processed_data, results)